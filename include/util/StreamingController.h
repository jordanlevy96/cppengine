/**
 * @file StreamingController.h
 * @brief Async tile loading from disk with bandwidth-limited GPU uploads via PBOs
 * @lines ~180
 *
 * Quick-stats (Public API):
 * - Initialize() - Setup loader thread, PBOs, paths (line ~85)
 * - Shutdown() - Stop loader thread gracefully (line ~95)
 * - RequestTile() - Queue tile for async loading (line ~100)
 * - ProcessUploads() - Upload tiles to GPU via PBO (line ~110)
 * - GetRequiredTiles() - Determine tiles needed for camera (line ~120)
 * - HasPendingUploads() - Check if tiles ready for upload (line ~130)
 *
 * CRITICAL: Multi-threaded architecture
 * - Main thread: RequestTile(), ProcessUploads(), GetRequiredTiles()
 * - Loader thread: Disk I/O, decompression
 * - Thread safety via m_requestMutex and m_loadedMutex
 */

#pragma once

#include "util/TileCache.h"
#include "util/ClipmapGeometry.h"

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

/**
 * @brief Tile load request queued for async loading
 */
struct TileRequest
{
    TileCoord coord;       ///< Tile to load
    int priority;          ///< Lower = higher priority (distance to camera)
    std::string filePath;  ///< Path to tile data file
};

/**
 * @brief Comparator for priority queue (lower priority value = higher priority)
 */
struct TileRequestComparator
{
    bool operator()(const TileRequest& a, const TileRequest& b) const
    {
        return a.priority > b.priority;
    }
};

/**
 * @brief Async tile streaming controller with bandwidth-limited GPU uploads
 *
 * Manages background loading of terrain tiles from disk,
 * and uploads them to GPU via PBO ring buffer to avoid stalls.
 *
 * **Thread Safety:**
 * - Main thread: RequestTile(), ProcessUploads(), GetRequiredTiles()
 * - Loader thread: Disk I/O, decompression, writes to m_loadedTiles
 * - Double-buffered via mutex-protected queues
 */
class StreamingController
{
public:
    StreamingController() = default;
    ~StreamingController();

    // Prevent copying
    StreamingController(const StreamingController&) = delete;
    StreamingController& operator=(const StreamingController&) = delete;

    /**
     * @brief Initialize streaming system
     * @param heightTilesPath Base path to height tile files
     * @param biomeTilesPath Base path to biome tile files
     * @param tileSize Tile dimension in pixels (e.g., 512)
     * @param maxBytesPerFrame Maximum bytes to upload per frame
     */
    void Initialize(const std::string& heightTilesPath,
                    const std::string& biomeTilesPath,
                    int tileSize,
                    size_t maxBytesPerFrame);

    /**
     * @brief Shutdown streaming system
     */
    void Shutdown();

    /**
     * @brief Queue a tile for async loading
     * @param coord Tile coordinate to load
     * @param priority Priority value (lower = higher priority)
     */
    void RequestTile(const TileCoord& coord, int priority);

    /**
     * @brief Check if tiles are ready for GPU upload
     * @return True if loaded tiles are pending upload
     */
    bool HasPendingUploads() const;

    /**
     * @brief Process pending tile uploads to GPU
     * @param gpuCache GPU tile cache to upload to
     * @param cpuCache CPU tile cache to store decoded tiles
     * @return Number of tiles uploaded this call
     */
    int ProcessUploads(GPUTileCache& gpuCache, CPUTileCache& cpuCache);

    /**
     * @brief Determine tiles needed for current camera position
     * @param cameraPos Camera world position
     * @param rings Clipmap ring configurations
     * @param mapWidth World map width in tiles
     * @param mapHeight World map height in tiles
     * @param wrapHorizontal Enable horizontal wrap-around
     * @return Vector of required tile coordinates not in cache
     */
    std::vector<TileCoord> GetRequiredTiles(const glm::vec3& cameraPos,
                                            const std::vector<ClipmapRing>& rings,
                                            int mapWidth, int mapHeight,
                                            bool wrapHorizontal);

    /**
     * @brief Get bytes uploaded this frame
     */
    size_t GetBytesUploadedThisFrame() const { return m_bytesThisFrame; }

    /**
     * @brief Get number of pending tile requests
     */
    int GetPendingRequestCount() const;

    /**
     * @brief Check if streaming system is initialized
     */
    bool IsInitialized() const { return m_running.load(); }

private:
    void LoaderThreadFunc();
    std::unique_ptr<CPUTile> LoadTileFromDisk(const TileRequest& request);
    std::unique_ptr<CPUTile> PopLoadedTile();
    std::string BuildTilePath(const std::string& basePath, const TileCoord& coord);

    // Loader thread
    std::thread m_loaderThread;
    mutable std::mutex m_requestMutex;
    std::condition_variable m_requestCV;
    std::priority_queue<TileRequest, std::vector<TileRequest>, TileRequestComparator> m_requestQueue;
    std::atomic<bool> m_running{false};

    // Loaded tiles ready for GPU upload
    mutable std::mutex m_loadedMutex;
    std::queue<std::unique_ptr<CPUTile>> m_loadedTiles;

    // PBO ring buffer for async uploads
    static constexpr int NUM_PBOS = 3;
    GLuint m_pbos[NUM_PBOS] = {0};
    int m_currentPBO = 0;

    // Configuration
    std::string m_heightPath;
    std::string m_biomePath;
    int m_tileSize = 512;
    size_t m_maxBytesPerFrame = 10 * 1024 * 1024;
    size_t m_bytesThisFrame = 0;

    bool m_isShutdown = false;
};

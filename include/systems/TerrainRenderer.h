/**
 * @file TerrainRenderer.h
 * @brief Streaming clipmap-based terrain system
 * @lines ~150
 *
 * Purpose: Renders large-scale terrain using clipmap LOD with async tile streaming.
 * Supports runtime height edits, horizontal wrap, and origin rebasing.
 *
 * Quick-stats (Public API):
 * - Initialize() - Setup terrain subsystems (line ~75)
 * - Shutdown() - Cleanup resources (line ~80)
 * - Update() - Per-frame tile streaming and dirty rect processing (line ~85)
 * - Render() - Draw clipmap rings (line ~90)
 * - MarkHeightDelta() - Queue additive height edit (line ~95)
 * - MarkHeightSet() - Queue absolute height edit (line ~100)
 * - GetHeightAt() - Sample height at world position (line ~105)
 * - GetStats() - Return debug statistics (line ~110)
 * - SetWireframe() - Toggle wireframe debug mode (line ~115)
 *
 * Thread safety:
 * - MarkHeightDelta/MarkHeightSet are thread-safe (mutex-protected queue)
 * - GetHeightAt reads from CPU cache (main thread only)
 * - All other methods: main thread only
 */

#pragma once

#include "util/TerrainConfig.h"
#include "util/ClipmapGeometry.h"
#include "util/TileCache.h"
#include "util/StreamingController.h"
#include "util/DirtyRectQueue.h"
#include "util/Shader.h"
#include "Camera.h"

#include <glm/glm.hpp>
#include <memory>

/**
 * @brief Debug statistics for terrain system
 */
struct TerrainStats
{
    int cpuCacheSize = 0;              ///< Number of tiles in CPU cache
    int gpuCacheSize = 0;              ///< Number of slots used in GPU cache
    int tilesLoaded = 0;               ///< Total tiles loaded this session
    int tilesUploaded = 0;             ///< Total tiles uploaded to GPU this session
    size_t bytesUploadedThisFrame = 0; ///< Bytes uploaded to GPU this frame
    int cacheHits = 0;                 ///< CPU cache hit count
    int cacheMisses = 0;               ///< CPU cache miss count
    int pendingDirtyRects = 0;         ///< Number of queued height edits
};

/**
 * @brief Streaming clipmap-based terrain renderer
 *
 * Singleton class that coordinates all terrain subsystems:
 * - ClipmapGeometry: Static ring meshes
 * - TileCache: CPU and GPU tile caching
 * - StreamingController: Async tile loading
 * - DirtyRectQueue: Runtime height edits
 *
 * **Usage:**
 * @code
 * // Initialization
 * TerrainConfig config;
 * config.mapWidth = 4096;
 * config.mapHeight = 2048;
 * TerrainRenderer::GetInstance().Initialize(config);
 *
 * // Each frame
 * TerrainRenderer::GetInstance().Update(camera, deltaTime);
 * TerrainRenderer::GetInstance().Render(camera);
 *
 * // Runtime edits from Lua
 * TerrainRenderer::GetInstance().MarkHeightDelta(100, 200, 50, 10, -5.0f);
 * @endcode
 */
class TerrainRenderer
{
public:
    /**
     * @brief Get singleton instance
     */
    static TerrainRenderer &GetInstance();

    /**
     * @brief Initialize terrain system
     * @param config Terrain configuration
     * @return True on success
     */
    bool Initialize(const TerrainConfig &config);

    /**
     * @brief Shutdown and release resources
     */
    void Shutdown();

    /**
     * @brief Per-frame update
     * @param cam Camera for LOD and streaming
     * @param delta Delta time in seconds
     */
    void Update(Camera *cam, float delta);

    /**
     * @brief Render terrain
     * @param cam Camera for view/projection matrices
     */
    void Render(Camera *cam);

    /**
     * @brief Queue additive height modification
     * @param x World X coordinate
     * @param y World Y coordinate
     * @param w Width in world units
     * @param h Height in world units
     * @param delta Height change amount
     * @note Thread-safe
     */
    void MarkHeightDelta(int x, int y, int w, int h, float delta);

    /**
     * @brief Queue absolute height set
     * @param x World X coordinate
     * @param y World Y coordinate
     * @param w Width in world units
     * @param h Height in world units
     * @param value Absolute height value
     * @note Thread-safe
     */
    void MarkHeightSet(int x, int y, int w, int h, float value);

    /**
     * @brief Sample height at world position
     * @param worldX World X coordinate
     * @param worldZ World Z coordinate
     * @return Height value, or 0 if tile not loaded
     */
    float GetHeightAt(float worldX, float worldZ) const;

    /**
     * @brief Get debug statistics
     * @return Current stats
     */
    TerrainStats GetStats() const;

    /**
     * @brief Toggle wireframe rendering
     * @param enabled True for wireframe
     */
    void SetWireframe(bool enabled) { m_wireframe = enabled; }

    /**
     * @brief Toggle clipmap ring visualization
     * @param enabled True to show ring boundaries
     */
    void SetShowClipRings(bool enabled) { m_showClipRings = enabled; }

    /**
     * @brief Check if terrain is initialized
     */
    bool IsInitialized() const { return m_initialized; }

private:
    TerrainRenderer() = default;
    ~TerrainRenderer() = default;
    TerrainRenderer(const TerrainRenderer &) = delete;
    TerrainRenderer &operator=(const TerrainRenderer &) = delete;

    // Subsystems
    ClipmapGeometry m_clipmap;
    CPUTileCache m_cpuCache;
    GPUTileCache m_gpuCache;
    StreamingController m_streaming;
    DirtyRectQueue m_dirtyQueue;
    std::unique_ptr<Shader> m_shader;

    // Configuration
    TerrainConfig m_config;

    // Origin rebasing (double precision for large worlds)
    glm::dvec3 m_worldOrigin{0.0, 0.0, 0.0};
    float m_sectorSize = 1000.0f;

    // State
    bool m_initialized = false;
    bool m_wireframe = false;
    bool m_showClipRings = false;

    // Stats
    mutable TerrainStats m_stats;
    int m_totalTilesLoaded = 0;
    int m_totalTilesUploaded = 0;
};

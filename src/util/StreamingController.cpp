/**
 * @file StreamingController.cpp
 * @brief Async tile loading from disk with bandwidth-limited GPU uploads via PBOs
 * @lines ~300
 *
 * Purpose: Manages background tile loading and GPU uploads for terrain streaming
 *
 * Key functions:
 * - Initialize() - Setup loader thread, PBOs, paths (line ~30, ~40 lines)
 * - Shutdown() - Stop loader thread gracefully (line ~75, ~25 lines)
 * - LoaderThreadFunc() - Background tile loading loop (line ~105, ~55 lines)
 * - ProcessUploads() - PBO-based GPU uploads (line ~165, ~70 lines)
 * - GetRequiredTiles() - Compute needed tiles for camera (line ~240, ~60 lines)
 */

#include "util/StreamingController.h"
#include "util/Logger.h"

#include <cmath>
#include <cstring>
#include <fstream>
#include <algorithm>

StreamingController::~StreamingController()
{
    Shutdown();
}

void StreamingController::Initialize(const std::string& heightTilesPath,
                                     const std::string& biomeTilesPath,
                                     int tileSize,
                                     size_t maxBytesPerFrame)
{
    if (m_running.load())
    {
        LOG_WARNING("[StreamingController] Already initialized");
        return;
    }

    m_heightPath = heightTilesPath;
    m_biomePath = biomeTilesPath;
    m_tileSize = tileSize;
    m_maxBytesPerFrame = maxBytesPerFrame;
    m_isShutdown = false;

    // Create PBO ring buffer for async uploads
    size_t pboSize = static_cast<size_t>(tileSize) * tileSize * sizeof(uint16_t);

    glGenBuffers(NUM_PBOS, m_pbos);
    for (int i = 0; i < NUM_PBOS; ++i)
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbos[i]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, pboSize, nullptr, GL_STREAM_DRAW);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    LOG_INFO("[StreamingController] Created {} PBOs of {} bytes each", NUM_PBOS, pboSize);

    // Start loader thread
    m_running.store(true);
    m_loaderThread = std::thread(&StreamingController::LoaderThreadFunc, this);

    LOG_INFO("[StreamingController] Initialized: tileSize={}, maxBytesPerFrame={}",
             tileSize, maxBytesPerFrame);
}

void StreamingController::Shutdown()
{
    if (m_isShutdown)
    {
        return;
    }
    m_isShutdown = true;

    // Signal loader thread to stop
    m_running.store(false);
    m_requestCV.notify_all();

    // Wait for loader thread to complete
    if (m_loaderThread.joinable())
    {
        m_loaderThread.join();
    }

    // Delete PBOs
    glDeleteBuffers(NUM_PBOS, m_pbos);
    for (int i = 0; i < NUM_PBOS; ++i)
    {
        m_pbos[i] = 0;
    }

    // Clear queues
    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        while (!m_requestQueue.empty())
        {
            m_requestQueue.pop();
        }
    }
    {
        std::lock_guard<std::mutex> lock(m_loadedMutex);
        while (!m_loadedTiles.empty())
        {
            m_loadedTiles.pop();
        }
    }

    LOG_INFO("[StreamingController] Shutdown complete");
}

void StreamingController::LoaderThreadFunc()
{
    LOG_INFO("[StreamingController] Loader thread started");

    while (m_running.load())
    {
        TileRequest request;
        bool hasRequest = false;

        // Wait for a request
        {
            std::unique_lock<std::mutex> lock(m_requestMutex);
            m_requestCV.wait(lock, [this]()
            {
                return !m_requestQueue.empty() || !m_running.load();
            });

            if (!m_running.load())
            {
                break;
            }

            if (!m_requestQueue.empty())
            {
                request = m_requestQueue.top();
                m_requestQueue.pop();
                hasRequest = true;
            }
        }

        if (hasRequest)
        {
            auto tile = LoadTileFromDisk(request);

            if (tile)
            {
                std::lock_guard<std::mutex> lock(m_loadedMutex);
                m_loadedTiles.push(std::move(tile));
                LOG_DEBUG("[StreamingController] Loaded tile ({}, {}, layer={})",
                          request.coord.x, request.coord.y, request.coord.layer);
            }
        }
    }

    LOG_INFO("[StreamingController] Loader thread stopped");
}

void StreamingController::RequestTile(const TileCoord& coord, int priority)
{
    std::lock_guard<std::mutex> lock(m_requestMutex);

    TileRequest request;
    request.coord = coord;
    request.priority = priority;
    request.filePath = BuildTilePath(m_heightPath, coord);

    m_requestQueue.push(request);
    m_requestCV.notify_one();
}

bool StreamingController::HasPendingUploads() const
{
    std::lock_guard<std::mutex> lock(m_loadedMutex);
    return !m_loadedTiles.empty();
}

int StreamingController::GetPendingRequestCount() const
{
    std::lock_guard<std::mutex> lock(m_requestMutex);
    return static_cast<int>(m_requestQueue.size());
}

std::unique_ptr<CPUTile> StreamingController::PopLoadedTile()
{
    std::lock_guard<std::mutex> lock(m_loadedMutex);
    if (m_loadedTiles.empty())
    {
        return nullptr;
    }
    auto tile = std::move(m_loadedTiles.front());
    m_loadedTiles.pop();
    return tile;
}

int StreamingController::ProcessUploads(GPUTileCache& gpuCache, CPUTileCache& cpuCache)
{
    int uploaded = 0;
    m_bytesThisFrame = 0;

    while (HasPendingUploads() && m_bytesThisFrame < m_maxBytesPerFrame)
    {
        auto tile = PopLoadedTile();
        if (!tile)
        {
            break;
        }

        size_t dataSize = tile->heightData.size() * sizeof(uint16_t);

        // Skip if this upload would exceed bandwidth limit
        if (m_bytesThisFrame + dataSize > m_maxBytesPerFrame && uploaded > 0)
        {
            std::lock_guard<std::mutex> lock(m_loadedMutex);
            m_loadedTiles.push(std::move(tile));
            break;
        }

        // Allocate GPU slot
        int slot = gpuCache.AllocateSlot(tile->coord);
        if (slot < 0)
        {
            LOG_ERROR("[StreamingController] No GPU slots for tile ({}, {}, layer={})",
                      tile->coord.x, tile->coord.y, tile->coord.layer);
            continue;
        }

        // Upload via PBO
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbos[m_currentPBO]);

        void* ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        if (ptr)
        {
            std::memcpy(ptr, tile->heightData.data(), dataSize);
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

            glBindTexture(GL_TEXTURE_2D_ARRAY, gpuCache.GetHeightTextureArray());
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                           0, 0, slot,
                           m_tileSize, m_tileSize, 1,
                           GL_RED, GL_UNSIGNED_SHORT,
                           nullptr);
        }

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        m_currentPBO = (m_currentPBO + 1) % NUM_PBOS;
        m_bytesThisFrame += dataSize;

        // Store in CPU cache
        cpuCache.Put(tile->coord, std::move(tile));
        uploaded++;
    }

    return uploaded;
}

std::vector<TileCoord> StreamingController::GetRequiredTiles(const glm::vec3& cameraPos,
                                                              const std::vector<ClipmapRing>& rings,
                                                              int mapWidth, int mapHeight,
                                                              bool wrapHorizontal)
{
    std::vector<TileCoord> required;

    for (size_t ringIndex = 0; ringIndex < rings.size(); ++ringIndex)
    {
        const ClipmapRing& ring = rings[ringIndex];

        // Ring covers a square centered on camera
        float halfExtent = static_cast<float>(ring.resolution) * ring.texelSize * 0.5f;

        float minX = cameraPos.x - halfExtent;
        float maxX = cameraPos.x + halfExtent;
        float minZ = cameraPos.z - halfExtent;
        float maxZ = cameraPos.z + halfExtent;

        // Tile world size is constant (tileSize in texels = tileSize in world units at base LOD)
        // Don't multiply by texelSize - tiles are always tileSize world units
        float tileWorldSize = static_cast<float>(m_tileSize);

        int tileMinX = static_cast<int>(std::floor(minX / tileWorldSize));
        int tileMaxX = static_cast<int>(std::floor(maxX / tileWorldSize));
        int tileMinZ = static_cast<int>(std::floor(minZ / tileWorldSize));
        int tileMaxZ = static_cast<int>(std::floor(maxZ / tileWorldSize));

        for (int tx = tileMinX; tx <= tileMaxX; ++tx)
        {
            for (int tz = tileMinZ; tz <= tileMaxZ; ++tz)
            {
                TileCoord coord;
                coord.layer = 0;  // Height layer

                if (wrapHorizontal)
                {
                    coord.x = ((tx % mapWidth) + mapWidth) % mapWidth;
                }
                else
                {
                    if (tx < 0 || tx >= mapWidth) continue;
                    coord.x = tx;
                }

                if (tz < 0 || tz >= mapHeight) continue;
                coord.y = tz;

                required.push_back(coord);
            }
        }
    }

    // Remove duplicates
    std::sort(required.begin(), required.end(),
              [](const TileCoord& a, const TileCoord& b)
              {
                  if (a.layer != b.layer) return a.layer < b.layer;
                  if (a.x != b.x) return a.x < b.x;
                  return a.y < b.y;
              });
    required.erase(std::unique(required.begin(), required.end()), required.end());

    return required;
}

std::unique_ptr<CPUTile> StreamingController::LoadTileFromDisk(const TileRequest& request)
{
    std::string heightPath = BuildTilePath(m_heightPath, request.coord);

    std::ifstream file(heightPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        // File doesn't exist - create empty tile (flat terrain)
        auto tile = std::make_unique<CPUTile>();
        tile->coord = request.coord;
        size_t numTexels = static_cast<size_t>(m_tileSize) * m_tileSize;
        tile->heightData.resize(numTexels, 0);
        return tile;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(fileSize);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
    {
        LOG_ERROR("[StreamingController] Failed to read file: {}", heightPath);
        return nullptr;
    }

    auto tile = std::make_unique<CPUTile>();
    tile->coord = request.coord;

    size_t numTexels = static_cast<size_t>(m_tileSize) * m_tileSize;
    tile->heightData.resize(numTexels);

    if (buffer.size() >= numTexels * sizeof(uint16_t))
    {
        std::memcpy(tile->heightData.data(), buffer.data(), numTexels * sizeof(uint16_t));
    }
    else
    {
        std::fill(tile->heightData.begin(), tile->heightData.end(), 0);
    }

    return tile;
}

std::string StreamingController::BuildTilePath(const std::string& basePath, const TileCoord& coord)
{
    char pathBuffer[512];
    std::snprintf(pathBuffer, sizeof(pathBuffer), "%s/tile_%d_%d.bin",
                  basePath.c_str(), coord.x, coord.y);
    return std::string(pathBuffer);
}

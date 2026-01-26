/**
 * @file TerrainRenderer.cpp
 * @brief Streaming clipmap-based terrain system implementation
 * @lines ~350
 *
 * Purpose: Coordinates terrain subsystems for large-world rendering
 *
 * Key functions:
 * - Initialize() - Setup all subsystems (line ~30, ~80 lines)
 * - Shutdown() - Cleanup (line ~115, ~20 lines)
 * - Update() - Tile streaming, dirty rects, origin rebasing (line ~140, ~70 lines)
 * - Render() - Draw all clipmap rings (line ~215, ~80 lines)
 * - GetHeightAt() - Sample CPU cache (line ~300, ~30 lines)
 */

#include "systems/TerrainRenderer.h"
#include "util/Logger.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

TerrainRenderer &TerrainRenderer::GetInstance()
{
    static TerrainRenderer instance;
    return instance;
}

bool TerrainRenderer::Initialize(const TerrainConfig &config)
{
    if (m_initialized)
    {
        LOG_WARNING("[TerrainRenderer] Already initialized");
        return true;
    }

    if (!config.IsValid())
    {
        LOG_ERROR("[TerrainRenderer] Invalid configuration");
        return false;
    }

    m_config = config;

    // Convert config rings to RingConfig format
    std::vector<RingConfig> ringConfigs;
    ringConfigs.reserve(config.rings.size());
    for (const auto &ring : config.rings)
    {
        ringConfigs.push_back({ring.resolution, ring.texelSize});
    }

    // Initialize clipmap geometry
    m_clipmap.Initialize(ringConfigs);
    LOG_INFO("[TerrainRenderer] Clipmap initialized with {} rings", ringConfigs.size());

    // Initialize CPU cache
    m_cpuCache.Initialize(config.cpuCacheSize, config.tileSize);

    // Initialize GPU cache
    m_gpuCache.Initialize(config.gpuCacheSize, config.tileSize,
                          config.mapWidth, config.mapHeight,
                          config.heightFormat, config.biomeFormat);

    // Initialize streaming controller
    m_streaming.Initialize(config.heightTilesPath, config.biomeTilesPath,
                           config.tileSize, config.maxUploadBytesPerFrame);

    // Load terrain shader
    try
    {
        m_shader = std::make_unique<Shader>("../res/shaders/Terrain.shader");
        LOG_INFO("[TerrainRenderer] Terrain shader loaded");
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("[TerrainRenderer] Failed to load shader: {}", e.what());
        return false;
    }

    m_initialized = true;
    LOG_INFO("[TerrainRenderer] Initialized successfully");
    LOG_INFO("  Map size: {}x{}", config.mapWidth, config.mapHeight);
    LOG_INFO("  Tile size: {}", config.tileSize);
    LOG_INFO("  CPU cache: {} tiles", config.cpuCacheSize);
    LOG_INFO("  GPU cache: {} slots", config.gpuCacheSize);

    return true;
}

void TerrainRenderer::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    LOG_INFO("[TerrainRenderer] Shutting down...");

    m_streaming.Shutdown();
    m_gpuCache.Shutdown();
    m_cpuCache.Shutdown();
    m_clipmap.Shutdown();
    m_shader.reset();

    m_initialized = false;
    LOG_INFO("[TerrainRenderer] Shutdown complete");
}

void TerrainRenderer::Update(Camera *cam, float delta)
{
    if (!m_initialized || !cam)
    {
        return;
    }

    // Advance frame for cache tracking
    m_cpuCache.AdvanceFrame();

    // Origin rebasing for large worlds
    glm::vec3 camPos = cam->transform.Pos;
    m_worldOrigin.x = std::floor(camPos.x / m_sectorSize) * m_sectorSize;
    m_worldOrigin.y = 0.0;
    m_worldOrigin.z = std::floor(camPos.z / m_sectorSize) * m_sectorSize;

    // Process dirty rects (runtime height edits)
    // DirtyRectQueue expects world dimensions in texels, not tile counts
    if (m_dirtyQueue.HasPending())
    {
        int worldWidth = m_config.mapWidth * m_config.tileSize;
        int worldHeight = m_config.mapHeight * m_config.tileSize;
        auto modifiedTiles = m_dirtyQueue.Flush(m_cpuCache, m_config.tileSize,
                                                worldWidth, worldHeight);

        // Re-upload modified tiles to GPU
        for (const auto &coord : modifiedTiles)
        {
            CPUTile *tile = m_cpuCache.Get(coord);
            if (tile && tile->dirty)
            {
                int slot = m_gpuCache.GetSlot(coord);
                if (slot >= 0)
                {
                    m_gpuCache.Upload(slot, tile);
                    tile->dirty = false;
                    m_totalTilesUploaded++;
                }
            }
        }
    }

    // Determine required tiles based on camera position
    std::vector<ClipmapRing> rings;
    for (int i = 0; i < m_clipmap.GetRingCount(); ++i)
    {
        rings.push_back(m_clipmap.GetRing(i));
    }

    // mapWidth/mapHeight are already tile counts, not texel dimensions
    auto requiredTiles = m_streaming.GetRequiredTiles(camPos, rings,
                                                      m_config.mapWidth,
                                                      m_config.mapHeight,
                                                      m_config.wrapHorizontal);

    // Debug: comprehensive logging every 2 seconds (120 frames at 60fps)
    static int frameCount = 0;
    if (frameCount++ % 120 == 0)
    {
        LOG_INFO("[TerrainRenderer] === Frame {} Status ===", frameCount);
        LOG_INFO("[TerrainRenderer] Camera: world=({:.1f}, {:.1f}, {:.1f})",
                 camPos.x, camPos.y, camPos.z);
        LOG_INFO("[TerrainRenderer] WorldOrigin: ({:.1f}, {:.1f}, {:.1f})",
                 m_worldOrigin.x, m_worldOrigin.y, m_worldOrigin.z);
        LOG_INFO("[TerrainRenderer] LocalCam: ({:.1f}, {:.1f}, {:.1f})",
                 camPos.x - m_worldOrigin.x, camPos.y - m_worldOrigin.y, camPos.z - m_worldOrigin.z);
        LOG_INFO("[TerrainRenderer] Tiles required: {}, CPU cache: {}, GPU slots: {}",
                 requiredTiles.size(),
                 m_cpuCache.GetTileCount(),
                 m_gpuCache.GetUsedSlotCount());

        // Log center tile info
        int centerTileX = static_cast<int>(camPos.x / m_config.tileSize);
        int centerTileZ = static_cast<int>(camPos.z / m_config.tileSize);
        LOG_INFO("[TerrainRenderer] Center tile: ({}, {}), slot: {}",
                 centerTileX, centerTileZ,
                 m_gpuCache.GetSlot({centerTileX, centerTileZ, 0}));
    }

    // Request tiles not in cache
    for (const auto &coord : requiredTiles)
    {
        if (!m_cpuCache.Get(coord))
        {
            // Calculate priority based on distance
            float dx = static_cast<float>(coord.x * m_config.tileSize) - camPos.x;
            float dz = static_cast<float>(coord.y * m_config.tileSize) - camPos.z;
            int priority = static_cast<int>(std::sqrt(dx * dx + dz * dz));

            m_streaming.RequestTile(coord, priority);
        }
    }

    // Process pending tile uploads
    int uploaded = m_streaming.ProcessUploads(m_gpuCache, m_cpuCache);
    m_totalTilesUploaded += uploaded;
    m_totalTilesLoaded += uploaded;
}

void TerrainRenderer::Render(Camera *cam)
{
    if (!m_initialized || !cam || !m_shader)
    {
        return;
    }

    m_shader->Use();

    // Disable depth writes so terrain renders purely as background
    glDepthMask(GL_FALSE);

    // Set matrices - terrain vertices are in local space (relative to worldOrigin)
    // so the view matrix must also use camera position relative to worldOrigin
    glm::vec3 cameraPos = cam->transform.Pos;
    glm::vec3 localCamPos = cameraPos - glm::vec3(m_worldOrigin);
    glm::mat4 View = glm::lookAt(localCamPos, localCamPos + cam->front, cam->up);
    m_shader->SetMat4("u_view", View);
    m_shader->SetMat4("u_projection", cam->Projection);

    // Set camera/world uniforms
    glm::vec3 camPos = cam->transform.Pos;
    m_shader->SetVec3("u_cameraPos", camPos);
    m_shader->SetVec3("u_worldOrigin", glm::vec3(m_worldOrigin));

    // Set terrain config uniforms
    m_shader->SetFloat("u_heightScale", m_config.heightScale);
    m_shader->SetFloat("u_heightOffset", m_config.heightOffset);
    // mapSize is in world units (tiles * tileSize), not tile counts
    float mapWorldWidth = static_cast<float>(m_config.mapWidth * m_config.tileSize);
    float mapWorldHeight = static_cast<float>(m_config.mapHeight * m_config.tileSize);
    m_shader->SetVec2("u_mapSize", glm::vec2(mapWorldWidth, mapWorldHeight));
    m_shader->SetInt("u_tileSize", m_config.tileSize);

    // Set fog uniforms
    m_shader->SetFloat("u_fogStart", m_config.fogStart);
    m_shader->SetFloat("u_fogEnd", m_config.fogEnd);
    m_shader->SetVec3("u_fogColor", m_config.fogColor);
    m_shader->SetVec3("u_lightDir", glm::normalize(m_config.lightDir));

    // Bind texture arrays
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_gpuCache.GetHeightTextureArray());
    m_shader->SetInt("u_heightTiles", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_gpuCache.GetHeightSlotMapTexture());
    m_shader->SetInt("u_heightSlotMap", 1);

    // Debug: log uniform values and coordinate verification
    static bool loggedUniforms = false;
    static int renderLogCount = 0;
    if (!loggedUniforms || renderLogCount++ % 120 == 0)
    {
        LOG_INFO("[TerrainRenderer] === Render Uniforms ===");
        LOG_INFO("[TerrainRenderer] World camera: ({:.1f}, {:.1f}, {:.1f})",
                 cameraPos.x, cameraPos.y, cameraPos.z);
        LOG_INFO("[TerrainRenderer] Local camera (view matrix): ({:.1f}, {:.1f}, {:.1f})",
                 localCamPos.x, localCamPos.y, localCamPos.z);
        LOG_INFO("[TerrainRenderer] WorldOrigin: ({:.1f}, {:.1f}, {:.1f})",
                 m_worldOrigin.x, m_worldOrigin.y, m_worldOrigin.z);
        LOG_INFO("[TerrainRenderer] Camera front: ({:.3f}, {:.3f}, {:.3f})",
                 cam->front.x, cam->front.y, cam->front.z);
        LOG_INFO("[TerrainRenderer] Map size: ({:.0f}, {:.0f}), heightScale: {:.1f}",
                 mapWorldWidth, mapWorldHeight, m_config.heightScale);
        LOG_INFO("[TerrainRenderer] Fog: start={:.1f}, end={:.1f}",
                 m_config.fogStart, m_config.fogEnd);
        LOG_INFO("[TerrainRenderer] Wireframe: {}", m_wireframe ? "ON" : "OFF");
        loggedUniforms = true;
    }

    // Set per-ring uniforms and draw
    for (int i = 0; i < m_clipmap.GetRingCount(); ++i)
    {
        const ClipmapRing &ring = m_clipmap.GetRing(i);
        glm::vec2 offset = m_clipmap.GetRingOffset(i, camPos);

        // Debug: log ring offsets periodically
        static int ringLogCount = 0;
        if (i == 0 && ringLogCount++ % 120 == 0)
        {
            LOG_INFO("[TerrainRenderer] === Ring Geometry (world coords) ===");
            for (int j = 0; j < m_clipmap.GetRingCount(); ++j)
            {
                const ClipmapRing &r = m_clipmap.GetRing(j);
                glm::vec2 off = m_clipmap.GetRingOffset(j, cameraPos);
                float extent = r.resolution * r.texelSize;
                LOG_INFO("[TerrainRenderer] Ring {}: offset=({:.1f},{:.1f}), extent={:.0f}, "
                         "covers ({:.1f},{:.1f}) to ({:.1f},{:.1f})",
                         j, off.x, off.y, extent,
                         off.x, off.y, off.x + extent, off.y + extent);
                // Show local space (after worldOrigin subtraction)
                glm::vec2 localOff = off - glm::vec2(m_worldOrigin.x, m_worldOrigin.z);
                LOG_INFO("[TerrainRenderer] Ring {} local: ({:.1f},{:.1f}) to ({:.1f},{:.1f})",
                         j, localOff.x, localOff.y, localOff.x + extent, localOff.y + extent);
            }
        }

        // Set ring-specific uniforms
        std::string prefix = "u_ringOffset[" + std::to_string(i) + "]";
        m_shader->SetVec2(prefix, offset);

        prefix = "u_ringTexelSize[" + std::to_string(i) + "]";
        m_shader->SetFloat(prefix, ring.texelSize);
    }

    // Draw all rings
    if (m_wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    // Debug: log draw calls once
    static bool loggedOnce = false;
    if (!loggedOnce)
    {
        LOG_INFO("[TerrainRenderer] Drawing {} rings", m_clipmap.GetRingCount());
        for (int i = 0; i < m_clipmap.GetRingCount(); ++i)
        {
            const ClipmapRing &ring = m_clipmap.GetRing(i);
            LOG_INFO("[TerrainRenderer] Ring {}: VAO={}, indexCount={}, texelSize={}",
                     i, ring.VAO, ring.indexCount, ring.texelSize);
        }
        loggedOnce = true;
    }

    for (int i = 0; i < m_clipmap.GetRingCount(); ++i)
    {
        const ClipmapRing &ring = m_clipmap.GetRing(i);

        glBindVertexArray(ring.VAO);
        glDrawElements(GL_TRIANGLES, ring.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    if (m_wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Restore depth writes for subsequent rendering
    glDepthMask(GL_TRUE);
}

void TerrainRenderer::MarkHeightDelta(int x, int y, int w, int h, float delta)
{
    float normalized = (m_config.heightScale > 0.0f) ? (delta / m_config.heightScale) : 0.0f;
    m_dirtyQueue.PushDelta(x, y, w, h, normalized);
}

void TerrainRenderer::MarkHeightSet(int x, int y, int w, int h, float value)
{
    float normalized = (m_config.heightScale > 0.0f) ? (value / m_config.heightScale) : 0.0f;
    m_dirtyQueue.PushSet(x, y, w, h, normalized);
}

float TerrainRenderer::GetHeightAt(float worldX, float worldZ) const
{
    if (!m_initialized)
    {
        return 0.0f;
    }

    // Convert world position to tile coordinates
    int tileX = static_cast<int>(std::floor(worldX / m_config.tileSize));
    int tileY = static_cast<int>(std::floor(worldZ / m_config.tileSize));

    // Handle wrap (mapWidth is tile count, not texel count)
    if (m_config.wrapHorizontal)
    {
        tileX = ((tileX % m_config.mapWidth) + m_config.mapWidth) % m_config.mapWidth;
    }

    TileCoord coord;
    coord.x = tileX;
    coord.y = tileY;
    coord.layer = 0;

    // Look up in CPU cache (const_cast needed for Get which updates LRU)
    CPUTile *tile = const_cast<CPUTileCache &>(m_cpuCache).Get(coord);
    if (!tile)
    {
        return 0.0f;
    }

    // Local position within tile
    int localX = static_cast<int>(worldX) % m_config.tileSize;
    int localY = static_cast<int>(worldZ) % m_config.tileSize;
    if (localX < 0)
        localX += m_config.tileSize;
    if (localY < 0)
        localY += m_config.tileSize;

    uint16_t heightRaw = tile->GetHeight(localX, localY, m_config.tileSize);
    return HeightU16ToFloat01(heightRaw) * m_config.heightScale;
}

TerrainStats TerrainRenderer::GetStats() const
{
    m_stats.cpuCacheSize = m_cpuCache.GetTileCount();
    m_stats.gpuCacheSize = m_gpuCache.GetUsedSlotCount();
    m_stats.tilesLoaded = m_totalTilesLoaded;
    m_stats.tilesUploaded = m_totalTilesUploaded;
    m_stats.bytesUploadedThisFrame = m_streaming.GetBytesUploadedThisFrame();
    m_stats.cacheHits = m_cpuCache.GetCacheHits();
    m_stats.cacheMisses = m_cpuCache.GetCacheMisses();
    m_stats.pendingDirtyRects = m_dirtyQueue.GetPendingCount();
    return m_stats;
}

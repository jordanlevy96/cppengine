/**
 * @file TiledBackgroundRenderer.cpp
 * @brief World-centric tiled background renderer (procedural tiles + GPU cache)
 * @lines ~520
 *
 * Purpose:
 * - Renders a ground plane behind the scene by drawing a set of stable world tiles.
 * - Tiles are procedurally generated on CPU (RGBA8) and cached on GPU in a texture array.
 *
 * Key functions:
 * - Initialize() - Create shader, VAO/VBO, and texture array cache (line ~40, ~120 lines)
 * - Render() - Select tiles, budget uploads, render instanced quads (line ~165, ~170 lines)
 * - GenerateTileRGBA8() - Procedural neon grid tile generator (line ~360, ~130 lines)
 *
 * Design notes:
 * - Avoids camera-centric clipmaps; tile keys are stable (lod,x,y).
 * - Current milestone uses lod=0 only; quadtree selection comes next.
 */

#include "systems/TiledBackgroundRenderer.h"

#include "controllers/Game.h"
#include "util/Logger.h"
#include "util/Shader.h"

#include <glad/glad.h>

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <limits>

namespace
{
    float SafeTanHalfFovRadians(float fovDegrees)
    {
        float clamped = std::clamp(fovDegrees, 1.0f, 179.0f);
        return std::tan(glm::radians(clamped) * 0.5f);
    }

    float PositiveMod(float x, float m)
    {
        float r = std::fmod(x, m);
        if (r < 0.0f)
        {
            r += m;
        }
        return r;
    }

    float Clamp01(float x)
    {
        return std::clamp(x, 0.0f, 1.0f);
    }

    int ClampLodIndex(int lod, int lodCount)
    {
        if (lodCount <= 1)
        {
            return 0;
        }
        return std::clamp(lod, 0, lodCount - 1);
    }
}

TiledBackgroundRenderer::~TiledBackgroundRenderer() = default;

std::uint64_t TiledBackgroundRenderer::PackKey(const TileKey &key)
{
    // Pack 3 signed 21-bit values into 64-bit: [lod:10][x:27][y:27] (biased).
    // This is intentionally simple; we can replace with a proper hash later.
    const std::uint64_t lod = static_cast<std::uint64_t>(key.lod & 0x3FF);
    const std::uint64_t bx = static_cast<std::uint64_t>(key.x + (1 << 26)) & 0x7FFFFFF;
    const std::uint64_t by = static_cast<std::uint64_t>(key.y + (1 << 26)) & 0x7FFFFFF;
    return (lod << 54) | (bx << 27) | by;
}

bool TiledBackgroundRenderer::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    // Require OpenGL context.
    glGetString(GL_VERSION);

    const std::string &res = Game::GetInstance().conf.ResourcePath;
    const std::string shaderPath = res + "shaders/TiledBackground.shader";

    try
    {
        m_shader = std::make_unique<Shader>(shaderPath);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("TiledBackgroundRenderer: failed to load shader {} ({})", shaderPath, e.what());
        return false;
    }

    EnsureDrawResources();
    m_initialized = true;
    RecreateCacheIfNeeded();
    LOG_INFO("TiledBackgroundRenderer initialized (tileRes={}, cacheSlots={})",
             m_config.tileResolution, m_config.cacheSlots);
    return true;
}

bool TiledBackgroundRenderer::EnsureInitialized()
{
    if (!m_initialized)
    {
        return Initialize();
    }
    return true;
}

void TiledBackgroundRenderer::Shutdown()
{
    if (m_tileTextureArray != 0)
    {
        glDeleteTextures(1, &m_tileTextureArray);
        m_tileTextureArray = 0;
    }

    if (m_instanceVbo != 0)
    {
        glDeleteBuffers(1, &m_instanceVbo);
        m_instanceVbo = 0;
    }
    if (m_vbo != 0)
    {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao != 0)
    {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }

    m_shader.reset();

    m_slots.clear();
    m_keyToSlot.clear();
    m_pendingUploads.clear();

    m_initialized = false;
}

void TiledBackgroundRenderer::Configure(const TiledBackgroundConfig &config)
{
    m_config = config;
    m_cacheDirty = true;
    m_warnedTileOverflow = false;

    if (m_config.tileResolution < 4)
    {
        m_config.tileResolution = 4;
    }
    if (m_config.cacheSlots < 1)
    {
        m_config.cacheSlots = 1;
    }
    if (m_config.maxUploadsPerFrame < 0)
    {
        m_config.maxUploadsPerFrame = 0;
    }
    if (m_config.tileWorldSize <= 0.0f)
    {
        m_config.tileWorldSize = 1.0f;
    }
    if (m_config.lodCount < 1)
    {
        m_config.lodCount = 1;
    }
    if (m_config.lodScale < 1.01f)
    {
        m_config.lodScale = 2.0f;
    }
    if (m_config.lodSplitFactor < 0.1f)
    {
        m_config.lodSplitFactor = 0.1f;
    }

    EnsureInitialized();
    RecreateCacheIfNeeded();
}

void TiledBackgroundRenderer::SetEnabled(bool enabled)
{
    m_config.enabled = enabled;
    if (enabled)
    {
        EnsureInitialized();
    }
}

float TiledBackgroundRenderer::GetTileWorldSizeForLOD(int lod) const
{
    const int clamped = ClampLodIndex(lod, m_config.lodCount);
    return m_config.tileWorldSize * std::pow(m_config.lodScale, static_cast<float>(clamped));
}

void TiledBackgroundRenderer::RecreateCacheIfNeeded()
{
    if (!m_cacheDirty)
    {
        return;
    }

    if (!EnsureInitialized())
    {
        return;
    }

    if (m_tileTextureArray != 0)
    {
        glDeleteTextures(1, &m_tileTextureArray);
        m_tileTextureArray = 0;
    }

    glGenTextures(1, &m_tileTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_tileTextureArray);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Allocate storage (no initial data).
    glTexImage3D(GL_TEXTURE_2D_ARRAY,
                 0,
                 GL_RGBA8,
                 m_config.tileResolution,
                 m_config.tileResolution,
                 m_config.cacheSlots,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 nullptr);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    m_slots.assign(static_cast<size_t>(m_config.cacheSlots), TileSlot{});
    m_keyToSlot.clear();
    m_pendingUploads.clear();

    m_cacheDirty = false;
}

void TiledBackgroundRenderer::SelectVisibleTiles(Camera *camera, std::vector<TileKey> &outKeys)
{
    outKeys.clear();

    if (!camera)
    {
        return;
    }

    // Conservative view region: oriented rectangle in XZ projected onto an AABB.
    const glm::vec3 camPos = camera->transform.Pos;
    glm::vec3 forward = camera->front;
    forward.y = 0.0f;
    const float forwardLen = glm::length(forward);
    if (forwardLen < 0.0001f)
    {
        forward = glm::vec3(0.0f, 0.0f, -1.0f);
    }
    else
    {
        forward /= forwardLen;
    }

    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

    const float aspect = camera->Projection[1][1] != 0.0f ? (camera->Projection[1][1] / camera->Projection[0][0]) : 1.0f;
    const float tanHalfFov = SafeTanHalfFovRadians(camera->fov);
    const float baseHalfWidth = m_config.farDistance * tanHalfFov * aspect * m_config.widthMultiplier;

    const glm::vec2 originXZ(camPos.x, camPos.z);
    const glm::vec2 fwdXZ(forward.x, forward.z);
    const glm::vec2 rightXZ(right.x, right.z);

    const glm::vec2 corners[4] = {
        originXZ + rightXZ * (-baseHalfWidth),
        originXZ + rightXZ * (baseHalfWidth),
        originXZ + fwdXZ * (m_config.farDistance) + rightXZ * (-baseHalfWidth),
        originXZ + fwdXZ * (m_config.farDistance) + rightXZ * (baseHalfWidth)};

    float minX = std::numeric_limits<float>::infinity();
    float maxX = -std::numeric_limits<float>::infinity();
    float minZ = std::numeric_limits<float>::infinity();
    float maxZ = -std::numeric_limits<float>::infinity();

    for (const glm::vec2 &c : corners)
    {
        minX = std::min(minX, c.x);
        maxX = std::max(maxX, c.x);
        minZ = std::min(minZ, c.y);
        maxZ = std::max(maxZ, c.y);
    }

    struct Candidate
    {
        TileKey key{};
        float forwardDist = 0.0f;
        float lateralDist = 0.0f;
    };

    std::vector<Candidate> candidates;

    struct Node
    {
        int lod = 0;
        int x = 0;
        int y = 0;
    };

    const int maxLod = std::max(0, m_config.lodCount - 1);
    const float rootTileSize = GetTileWorldSizeForLOD(maxLod);
    const float invRootSize = 1.0f / rootTileSize;

    const int rootX0 = static_cast<int>(std::floor(minX * invRootSize));
    const int rootX1 = static_cast<int>(std::floor(maxX * invRootSize));
    const int rootY0 = static_cast<int>(std::floor(minZ * invRootSize));
    const int rootY1 = static_cast<int>(std::floor(maxZ * invRootSize));

    // Cap selection to avoid runaway in case of bad config.
    const int maxTilesPerAxis = 128;
    const int clampedX0 = std::max(rootX0, rootX1 - maxTilesPerAxis);
    const int clampedX1 = std::min(rootX1, rootX0 + maxTilesPerAxis);
    const int clampedY0 = std::max(rootY0, rootY1 - maxTilesPerAxis);
    const int clampedY1 = std::min(rootY1, rootY0 + maxTilesPerAxis);

    std::vector<Node> stack;
    stack.reserve(static_cast<size_t>((clampedX1 - clampedX0 + 1) * (clampedY1 - clampedY0 + 1)));

    for (int ty = clampedY0; ty <= clampedY1; ty++)
    {
        for (int tx = clampedX0; tx <= clampedX1; tx++)
        {
            stack.push_back(Node{.lod = maxLod, .x = tx, .y = ty});
        }
    }

    while (!stack.empty())
    {
        const Node n = stack.back();
        stack.pop_back();

        const float tileSize = GetTileWorldSizeForLOD(n.lod);
        const glm::vec2 center((static_cast<float>(n.x) + 0.5f) * tileSize,
                               (static_cast<float>(n.y) + 0.5f) * tileSize);
        const glm::vec2 rel = center - originXZ;
        const float fwdDist = glm::dot(rel, fwdXZ);

        const float behindEpsilon = -tileSize;
        if (fwdDist < behindEpsilon)
        {
            continue;
        }

        const float latDist = std::abs(glm::dot(rel, rightXZ));

        const bool canSplit = (n.lod > 0);
        const float splitThreshold = tileSize * m_config.lodSplitFactor;
        const bool shouldSplit = (fwdDist >= 0.0f && fwdDist < splitThreshold);

        if (canSplit && shouldSplit)
        {
            const int childLod = n.lod - 1;
            const int cx = n.x * 2;
            const int cy = n.y * 2;
            stack.push_back(Node{.lod = childLod, .x = cx + 0, .y = cy + 0});
            stack.push_back(Node{.lod = childLod, .x = cx + 1, .y = cy + 0});
            stack.push_back(Node{.lod = childLod, .x = cx + 0, .y = cy + 1});
            stack.push_back(Node{.lod = childLod, .x = cx + 1, .y = cy + 1});
            continue;
        }

        candidates.push_back(Candidate{
            .key = TileKey{n.lod, n.x, n.y},
            .forwardDist = fwdDist,
            .lateralDist = latDist});
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate &a, const Candidate &b)
              {
                  if (a.forwardDist != b.forwardDist)
                  {
                      return a.forwardDist < b.forwardDist;
                  }
                  if (a.lateralDist != b.lateralDist)
                  {
                      return a.lateralDist < b.lateralDist;
                  }
                  if (a.key.y != b.key.y)
                  {
                      return a.key.y < b.key.y;
                  }
                  return a.key.x < b.key.x;
              });

    const int maxKeys = std::max(1, m_config.cacheSlots);
    const int candidateCount = static_cast<int>(candidates.size());
    if (candidateCount > maxKeys)
    {
        if (!m_warnedTileOverflow)
        {
            LOG_WARNING("TiledBackgroundRenderer: visible tiles ({}) exceed cacheSlots ({}); truncating selection (consider increasing cacheSlots or reducing farDistance/widthMultiplier)",
                        candidateCount, maxKeys);
            m_warnedTileOverflow = true;
        }
        candidates.resize(static_cast<size_t>(maxKeys));
    }

    outKeys.reserve(candidates.size());
    for (const Candidate &c : candidates)
    {
        outKeys.push_back(c.key);
    }

    m_lastStats.visibleCandidates = candidateCount;
    m_lastStats.selectedTiles = static_cast<int>(outKeys.size());
}

int TiledBackgroundRenderer::ResolveTileLayer(const TileKey &key, bool &outIsNew)
{
    outIsNew = false;

    const std::uint64_t packed = PackKey(key);
    auto it = m_keyToSlot.find(packed);
    if (it != m_keyToSlot.end())
    {
        const int slot = it->second;
        m_slots[slot].lastUsedFrame = m_frameIndex;
        m_lastStats.cacheHits++;
        return slot;
    }
    m_lastStats.cacheMisses++;

    // Find free slot or evict LRU.
    int chosen = -1;
    for (int i = 0; i < static_cast<int>(m_slots.size()); i++)
    {
        if (!m_slots[i].occupied)
        {
            chosen = i;
            break;
        }
    }

    if (chosen < 0)
    {
        std::uint64_t bestFrame = std::numeric_limits<std::uint64_t>::max();
        for (int i = 0; i < static_cast<int>(m_slots.size()); i++)
        {
            if (m_slots[i].lastUsedFrame < bestFrame)
            {
                bestFrame = m_slots[i].lastUsedFrame;
                chosen = i;
            }
        }
        if (chosen >= 0)
        {
            const std::uint64_t evicted = PackKey(m_slots[chosen].key);
            m_keyToSlot.erase(evicted);
            m_lastStats.evictions++;
        }
    }

    if (chosen < 0)
    {
        return 0;
    }

    m_slots[chosen].occupied = true;
    m_slots[chosen].key = key;
    m_slots[chosen].lastUsedFrame = m_frameIndex;
    m_slots[chosen].uploaded = false;
    m_keyToSlot[packed] = chosen;

    outIsNew = true;
    return chosen;
}

void TiledBackgroundRenderer::UploadPendingTiles()
{
    if (m_pendingUploads.empty() || m_config.maxUploadsPerFrame == 0)
    {
        return;
    }

    const int budget = std::min(m_config.maxUploadsPerFrame, static_cast<int>(m_pendingUploads.size()));
    if (budget <= 0)
    {
        return;
    }

    std::vector<std::uint8_t> pixels;
    pixels.reserve(static_cast<size_t>(m_config.tileResolution * m_config.tileResolution * 4));

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_tileTextureArray);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (int i = 0; i < budget; i++)
    {
        const TileKey key = m_pendingUploads.back();
        m_pendingUploads.pop_back();

        const std::uint64_t packed = PackKey(key);
        auto it = m_keyToSlot.find(packed);
        if (it == m_keyToSlot.end())
        {
            continue;
        }
        const int slot = it->second;

        GenerateTileRGBA8(key, pixels);

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                        0,
                        0,
                        0,
                        slot,
                        m_config.tileResolution,
                        m_config.tileResolution,
                        1,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        pixels.data());

        if (slot >= 0 && slot < static_cast<int>(m_slots.size()))
        {
            m_slots[slot].uploaded = true;
        }

        m_lastStats.uploadsThisFrame++;
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void TiledBackgroundRenderer::EnsureDrawResources()
{
    if (m_vao != 0)
    {
        return;
    }

    // Unit quad in XZ plane with UVs (two triangles). Vertex format: pos2, uv2.
    const float quad[] = {
        // x, z, u, v
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f};

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_instanceVbo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    // location 0: local pos (x,z)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)0);
    glEnableVertexAttribArray(0);

    // location 1: uv
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);

    // Instance data: originXZ (vec2) + layer (float) + hasData (float) + tileWorldSize (float)
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(TileInstance) * 1, nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TileInstance), (void *)offsetof(TileInstance, originXZ));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(TileInstance), (void *)offsetof(TileInstance, layer));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(TileInstance), (void *)offsetof(TileInstance, hasData));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(TileInstance), (void *)offsetof(TileInstance, tileWorldSize));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    m_instanceCapacity = 1;
}

void TiledBackgroundRenderer::DrawTiles(Camera *camera, const std::vector<TileInstance> &instances) const
{
    if (instances.empty() || !m_shader)
    {
        return;
    }

    m_shader->Use();

    const glm::vec3 cameraPos = camera->transform.Pos;
    const glm::mat4 view = glm::lookAt(cameraPos, cameraPos + camera->front, camera->up);

    m_shader->SetMat4("view", view);
    m_shader->SetMat4("projection", camera->Projection);
    m_shader->SetFloat("u_planeY", m_config.planeY);
    m_shader->SetVec4("u_fallbackColor", m_config.baseColorA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_tileTextureArray);
    m_shader->SetInt("u_tiles", 0);

    glBindVertexArray(m_vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(instances.size()));
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glUseProgram(0);
}

void TiledBackgroundRenderer::Render(Camera *camera, float deltaMs)
{
    (void)deltaMs;

    if (!m_config.enabled)
    {
        return;
    }

    if (!camera)
    {
        return;
    }

    if (!EnsureInitialized())
    {
        return;
    }

    RecreateCacheIfNeeded();

    m_frameIndex++;
    m_lastStats = Stats{};
    m_lastStats.frameIndex = m_frameIndex;

    std::vector<TileKey> visibleKeys;
    SelectVisibleTiles(camera, visibleKeys);

    std::vector<TileInstance> instances;
    instances.reserve(visibleKeys.size());

    for (const TileKey &key : visibleKeys)
    {
        bool isNew = false;
        const int slot = ResolveTileLayer(key, isNew);
        if (isNew)
        {
            m_pendingUploads.push_back(key);
        }

        const float tileSize = GetTileWorldSizeForLOD(key.lod);
        const glm::vec2 originXZ(key.x * tileSize, key.y * tileSize);
        instances.push_back(TileInstance{
            .originXZ = originXZ,
            .layer = static_cast<float>(slot),
            .hasData = 0.0f,
            .tileWorldSize = tileSize});
    }

    UploadPendingTiles();

    // Mark which instances have valid data.
    for (TileInstance &inst : instances)
    {
        const int slot = static_cast<int>(inst.layer);
        if (slot >= 0 && slot < static_cast<int>(m_slots.size()) && m_slots[slot].uploaded)
        {
            inst.hasData = 1.0f;
        }
        else
        {
            inst.hasData = 0.0f;
        }
    }

    // Grow instance buffer if needed.
    if (static_cast<int>(instances.size()) > m_instanceCapacity)
    {
        m_instanceCapacity = std::max(m_instanceCapacity * 2, static_cast<int>(instances.size()));
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(TileInstance) * m_instanceCapacity, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TileInstance) * instances.size(), instances.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Render behind everything else.
    const GLboolean wasCullEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean wasDepthMask = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &wasDepthMask);

    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    DrawTiles(camera, instances);

    if (wasCullEnabled)
    {
        glEnable(GL_CULL_FACE);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
    glDepthMask(wasDepthMask);

    int resident = 0;
    for (const TileSlot &s : m_slots)
    {
        if (s.occupied)
        {
            resident++;
        }
    }
    m_lastStats.residentTiles = resident;
    m_lastStats.pendingUploads = static_cast<int>(m_pendingUploads.size());

    const std::uint64_t logIntervalFrames = 120;
    if (m_frameIndex - m_lastStatsLogFrame >= logIntervalFrames)
    {
        m_lastStatsLogFrame = m_frameIndex;
        LOG_INFO("BackgroundTiles: sel={} cand={} res={} uploads={} pend={} hit={} miss={} evict={}",
                 m_lastStats.selectedTiles,
                 m_lastStats.visibleCandidates,
                 m_lastStats.residentTiles,
                 m_lastStats.uploadsThisFrame,
                 m_lastStats.pendingUploads,
                 m_lastStats.cacheHits,
                 m_lastStats.cacheMisses,
                 m_lastStats.evictions);
    }
}

void TiledBackgroundRenderer::GenerateTileRGBA8(const TileKey &key, std::vector<std::uint8_t> &outPixels) const
{
    const int w = m_config.tileResolution;
    const int h = m_config.tileResolution;

    outPixels.resize(static_cast<size_t>(w * h * 4));

    const float tileSize = GetTileWorldSizeForLOD(key.lod);
    const float grid = std::max(0.0001f, m_config.gridSpacing);
    const float majorGrid = grid * std::max(1, m_config.majorEvery);
    const float lineW = std::max(0.0f, m_config.lineWidth);

    const float x0 = key.x * tileSize;
    const float z0 = key.y * tileSize;

    for (int y = 0; y < h; y++)
    {
        const float v = (h == 1) ? 0.0f : (static_cast<float>(y) / static_cast<float>(h - 1));
        const float worldZ = z0 + v * tileSize;

        for (int x = 0; x < w; x++)
        {
            const float u = (w == 1) ? 0.0f : (static_cast<float>(x) / static_cast<float>(w - 1));
            const float worldX = x0 + u * tileSize;

            const float mx = PositiveMod(worldX, grid);
            const float mz = PositiveMod(worldZ, grid);
            const float distMinorX = std::min(mx, grid - mx);
            const float distMinorZ = std::min(mz, grid - mz);
            const float minorDist = std::min(distMinorX, distMinorZ);

            const float Mx = PositiveMod(worldX, majorGrid);
            const float Mz = PositiveMod(worldZ, majorGrid);
            const float distMajorX = std::min(Mx, majorGrid - Mx);
            const float distMajorZ = std::min(Mz, majorGrid - Mz);
            const float majorDist = std::min(distMajorX, distMajorZ);

            // Base gradient: dark -> brighter with distance along -Z (tuned for Tetris camera).
            const float gradT = Clamp01((-worldZ) * 0.0015f);
            glm::vec4 color = glm::mix(m_config.baseColorA, m_config.baseColorB, gradT);

            if (minorDist <= lineW)
            {
                const float k = 1.0f - Clamp01(minorDist / std::max(0.0001f, lineW));
                color = glm::mix(color, m_config.minorLineColor, k);
            }

            if (majorDist <= lineW * 1.5f)
            {
                const float k = 1.0f - Clamp01(majorDist / std::max(0.0001f, lineW * 1.5f));
                color = glm::mix(color, m_config.majorLineColor, k);
            }

            // Deterministic per-tile variation: subtle hue offset based on key.
            const float keyHash = std::fmod(std::abs(key.x * 12.9898f + key.y * 78.233f), 1.0f);
            const float tint = (0.92f + 0.08f * keyHash);
            color.r *= tint;
            color.g *= tint;
            color.b *= tint;

            const int idx = (y * w + x) * 4;
            outPixels[static_cast<size_t>(idx + 0)] = static_cast<std::uint8_t>(std::clamp(color.r, 0.0f, 1.0f) * 255.0f);
            outPixels[static_cast<size_t>(idx + 1)] = static_cast<std::uint8_t>(std::clamp(color.g, 0.0f, 1.0f) * 255.0f);
            outPixels[static_cast<size_t>(idx + 2)] = static_cast<std::uint8_t>(std::clamp(color.b, 0.0f, 1.0f) * 255.0f);
            outPixels[static_cast<size_t>(idx + 3)] = static_cast<std::uint8_t>(std::clamp(color.a, 0.0f, 1.0f) * 255.0f);
        }
    }
}

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
#include <queue>
#include <unordered_set>

namespace
{
    float SafeTanHalfFovRadians(float fovDegrees)
    {
        float clamped = std::clamp(fovDegrees, 1.0f, 179.0f);
        return std::tan(glm::radians(clamped) * 0.5f);
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

    std::uint32_t Hash2D(std::int32_t x, std::int32_t y)
    {
        // Deterministic integer hash (FNV-1a style). Good enough for background noise.
        std::uint32_t h = 2166136261u;
        h = (h ^ static_cast<std::uint32_t>(x)) * 16777619u;
        h = (h ^ static_cast<std::uint32_t>(y)) * 16777619u;
        return h;
    }

    float Hash01(std::int32_t x, std::int32_t y)
    {
        // 24-bit mantissa for stable [0..1) float conversion.
        const std::uint32_t h = Hash2D(x, y);
        return static_cast<float>(h & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
    }

    float Fade(float t)
    {
        // Smoothstep polynomial used in classic value noise.
        return t * t * (3.0f - 2.0f * t);
    }

    float Lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

    float ValueNoise2D(float x, float y)
    {
        const std::int32_t xi = static_cast<std::int32_t>(std::floor(x));
        const std::int32_t yi = static_cast<std::int32_t>(std::floor(y));
        const float xf = x - static_cast<float>(xi);
        const float yf = y - static_cast<float>(yi);

        const float a = Hash01(xi, yi);
        const float b = Hash01(xi + 1, yi);
        const float c = Hash01(xi, yi + 1);
        const float d = Hash01(xi + 1, yi + 1);

        const float u = Fade(xf);
        const float v = Fade(yf);

        return Lerp(Lerp(a, b, u), Lerp(c, d, u), v);
    }

    float Fbm2D(float x, float y, int octaves)
    {
        float v = 0.0f;
        float a = 0.5f;
        float f = 1.0f;
        for (int i = 0; i < octaves; i++)
        {
            v += a * ValueNoise2D(x * f, y * f);
            f *= 2.0f;
            a *= 0.5f;
        }
        return v;
    }

    float RidgedFbm2D(float x, float y, int octaves)
    {
        // Ridged multifractal style noise (good for mountain-like silhouettes).
        // Produces sharper peaks than plain FBM.
        float v = 0.0f;
        float a = 0.5f;
        float f = 1.0f;
        float weight = 1.0f;

        for (int i = 0; i < octaves; i++)
        {
            float n = ValueNoise2D(x * f, y * f);
            // Map [0..1] to ridges: peak at 0.5, valleys at 0 and 1.
            float ridge = 1.0f - std::abs(2.0f * n - 1.0f);
            ridge = ridge * ridge; // sharpen

            ridge *= weight;
            weight = std::clamp(ridge * 2.0f, 0.0f, 1.0f);

            v += ridge * a;
            f *= 2.0f;
            a *= 0.5f;
        }

        return v;
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

    if (m_heightTextureArray != 0)
    {
        glDeleteTextures(1, &m_heightTextureArray);
        m_heightTextureArray = 0;
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
    m_meshDirty = true;
    m_warnedTileOverflow = false;

    if (m_config.tileResolution < 4)
    {
        m_config.tileResolution = 4;
    }
    if (m_config.heightResolution < 1)
    {
        m_config.heightResolution = 1;
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
    // Current selection/subdivision assumes a quadtree (2x2 children), so lodScale must be 2.0.
    // If we want arbitrary lodScale later, we need to revisit indexing and subdivision math.
    if (std::abs(m_config.lodScale - 2.0f) > 0.001f)
    {
        LOG_WARNING("TiledBackgroundRenderer: lodScale={} is not supported (quadtree requires 2.0); clamping to 2.0", m_config.lodScale);
        m_config.lodScale = 2.0f;
    }
    if (m_config.lodSplitFactor < 0.1f)
    {
        m_config.lodSplitFactor = 0.1f;
    }

    if (m_config.meshResolution < 1)
    {
        m_config.meshResolution = 1;
    }

    m_config.horizonLinePixels = std::max(0.0f, m_config.horizonLinePixels);

    m_config.mountainExtraDistance = std::max(0.0f, m_config.mountainExtraDistance);
    m_config.mountainHeight = std::max(0.0f, m_config.mountainHeight);
    m_config.mountainNoiseScale = std::max(0.0f, m_config.mountainNoiseScale);
    m_config.mountainFeatureSize = std::max(0.0f, m_config.mountainFeatureSize);
    m_config.mountainDetail = std::clamp(m_config.mountainDetail, 0.0f, 1.0f);
    m_config.mountainFadeDistance = std::max(0.0f, m_config.mountainFadeDistance);
    m_config.mountainRiseExponent = std::clamp(m_config.mountainRiseExponent, 0.10f, 4.0f);
    m_config.skirtDepth = std::max(0.0f, m_config.skirtDepth);
    m_config.mountainGridScale = std::max(1.0f, m_config.mountainGridScale);
    m_config.mountainMajorStrength = std::clamp(m_config.mountainMajorStrength, 0.0f, 1.0f);

    m_config.mountainSideStrength = std::clamp(m_config.mountainSideStrength, 0.0f, 1.0f);
    m_config.mountainSideOffsetX = std::max(0.0f, m_config.mountainSideOffsetX);
    m_config.mountainSideWidthX = std::max(0.0001f, m_config.mountainSideWidthX);
    m_config.mountainSideBase = std::clamp(m_config.mountainSideBase, 0.0f, 1.0f);

    EnsureInitialized();
    RecreateCacheIfNeeded();
    EnsureDrawResources();

    const float derivedNoiseScale = (m_config.mountainFeatureSize > 0.0f) ? (1.0f / m_config.mountainFeatureSize) : m_config.mountainNoiseScale;
    LOG_INFO("BackgroundTiles: far={} extra={} height={} featureSize={} noiseScale={} fade={} riseExp={} skirtDepth={} mGridScale={} mMajor={}",
             m_config.farDistance,
             m_config.mountainExtraDistance,
             m_config.mountainHeight,
             m_config.mountainFeatureSize,
             derivedNoiseScale,
             m_config.mountainFadeDistance,
             m_config.mountainRiseExponent,
             m_config.skirtDepth,
             m_config.mountainGridScale,
             m_config.mountainMajorStrength);
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
    return m_config.tileWorldSize / std::pow(m_config.lodScale, static_cast<float>(clamped));
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

    if (m_heightTextureArray != 0)
    {
        glDeleteTextures(1, &m_heightTextureArray);
        m_heightTextureArray = 0;
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

    // Height tile cache (R16, normalized): used for vertex displacement / "elevation layer".
    glGenTextures(1, &m_heightTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_heightTextureArray);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage3D(GL_TEXTURE_2D_ARRAY,
                 0,
                 GL_R16,
                 m_config.heightResolution,
                 m_config.heightResolution,
                 m_config.cacheSlots,
                 0,
                 GL_RED,
                 GL_UNSIGNED_SHORT,
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
    const float maxDistance = std::max(0.0f, m_config.farDistance + m_config.mountainExtraDistance);
    const float baseHalfWidth = maxDistance * tanHalfFov * aspect * m_config.widthMultiplier;

    const glm::vec2 originXZ(camPos.x, camPos.z);
    const glm::vec2 fwdXZ(forward.x, forward.z);
    const glm::vec2 rightXZ(right.x, right.z);

    const glm::vec2 corners[4] = {
        originXZ + rightXZ * (-baseHalfWidth),
        originXZ + rightXZ * (baseHalfWidth),
        originXZ + fwdXZ * (maxDistance) + rightXZ * (-baseHalfWidth),
        originXZ + fwdXZ * (maxDistance) + rightXZ * (baseHalfWidth)};

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

    struct Node
    {
        int lod = 0;
        int x = 0;
        int y = 0;
    };

    auto computeCandidate = [&](const Node &n, Candidate &out) -> bool
    {
        const float tileSize = GetTileWorldSizeForLOD(n.lod);
        const glm::vec2 center((static_cast<float>(n.x) + 0.5f) * tileSize,
                               (static_cast<float>(n.y) + 0.5f) * tileSize);
        const glm::vec2 rel = center - originXZ;
        const float fwdDist = glm::dot(rel, fwdXZ);

        const float behindEpsilon = -tileSize;
        if (fwdDist < behindEpsilon)
        {
            return false;
        }

        const float latDist = std::abs(glm::dot(rel, rightXZ));
        out = Candidate{
            .key = TileKey{n.lod, n.x, n.y},
            .forwardDist = fwdDist,
            .lateralDist = latDist};
        return true;
    };

    auto shouldSplit = [&](const Candidate &c) -> bool
    {
        const int lod = c.key.lod;
        if (lod >= m_config.lodCount - 1)
        {
            return false;
        }

        const float tileSize = GetTileWorldSizeForLOD(lod);
        const float splitThreshold = tileSize * m_config.lodSplitFactor;
        return (c.forwardDist >= 0.0f && c.forwardDist < splitThreshold);
    };

    // Build a complete-coverage selection under a hard budget:
    // start from coarse tiles, then refine nearest tiles until budget is exhausted.
    const int rootLod = 0;
    const float rootTileSize = GetTileWorldSizeForLOD(rootLod);
    const float invRootSize = 1.0f / rootTileSize;

    const int rootX0 = static_cast<int>(std::floor(minX * invRootSize));
    const int rootX1 = static_cast<int>(std::floor(maxX * invRootSize));
    const int rootY0 = static_cast<int>(std::floor(minZ * invRootSize));
    const int rootY1 = static_cast<int>(std::floor(maxZ * invRootSize));

    // Cap root coverage in case of bad config.
    const int maxTilesPerAxis = 128;
    auto clampAxisSpan = [&](int minV, int maxV) -> std::pair<int, int>
    {
        if (minV > maxV)
        {
            std::swap(minV, maxV);
        }

        const int span = maxV - minV + 1;
        if (span <= maxTilesPerAxis)
        {
            return {minV, maxV};
        }

        const int half = maxTilesPerAxis / 2;
        int start = minV + span / 2 - half;
        int end = start + maxTilesPerAxis - 1;

        if (start < minV)
        {
            start = minV;
            end = start + maxTilesPerAxis - 1;
        }
        if (end > maxV)
        {
            end = maxV;
            start = end - maxTilesPerAxis + 1;
        }

        if (start > end)
        {
            start = minV;
            end = minV - 1;
        }

        return {start, end};
    };

    const auto [clampedX0, clampedX1] = clampAxisSpan(rootX0, rootX1);
    const auto [clampedY0, clampedY1] = clampAxisSpan(rootY0, rootY1);

    std::vector<Candidate> leaves;
    const int spanX = clampedX1 - clampedX0 + 1;
    const int spanY = clampedY1 - clampedY0 + 1;
    if (spanX <= 0 || spanY <= 0)
    {
        m_lastStats.visibleCandidates = 0;
        m_lastStats.selectedTiles = 0;
        return;
    }

    leaves.reserve(static_cast<size_t>(spanX * spanY));

    struct HeapItem
    {
        float priority = 0.0f; // higher = split earlier
        Candidate cand{};
    };

    struct HeapLess
    {
        bool operator()(const HeapItem &a, const HeapItem &b) const
        {
            if (a.priority != b.priority)
            {
                return a.priority < b.priority;
            }
            if (a.cand.forwardDist != b.cand.forwardDist)
            {
                return a.cand.forwardDist > b.cand.forwardDist;
            }
            if (a.cand.lateralDist != b.cand.lateralDist)
            {
                return a.cand.lateralDist > b.cand.lateralDist;
            }
            if (a.cand.key.lod != b.cand.key.lod)
            {
                return a.cand.key.lod < b.cand.key.lod;
            }
            if (a.cand.key.y != b.cand.key.y)
            {
                return a.cand.key.y > b.cand.key.y;
            }
            return a.cand.key.x > b.cand.key.x;
        }
    };

    std::priority_queue<HeapItem, std::vector<HeapItem>, HeapLess> heap;

    auto pushSplitCandidate = [&](const Candidate &c)
    {
        if (!shouldSplit(c))
        {
            return;
        }
        const float tileSize = GetTileWorldSizeForLOD(c.key.lod);
        const float splitThreshold = tileSize * m_config.lodSplitFactor;
        const float priority = (splitThreshold - c.forwardDist);
        heap.push(HeapItem{.priority = priority, .cand = c});
    };

    for (int ty = clampedY0; ty <= clampedY1; ty++)
    {
        for (int tx = clampedX0; tx <= clampedX1; tx++)
        {
            Candidate c{};
            if (!computeCandidate(Node{.lod = rootLod, .x = tx, .y = ty}, c))
            {
                continue;
            }
            leaves.push_back(c);
            pushSplitCandidate(c);
        }
    }

    const int maxKeys = std::max(1, m_config.cacheSlots);
    if (static_cast<int>(leaves.size()) > maxKeys)
    {
        if (!m_warnedTileOverflow)
        {
            LOG_WARNING("TiledBackgroundRenderer: root coverage ({}) exceeds cacheSlots ({}); consider increasing cacheSlots or increasing tileWorldSize/reducing farDistance",
                        leaves.size(), maxKeys);
            m_warnedTileOverflow = true;
        }
        leaves.resize(static_cast<size_t>(maxKeys));
        heap = {};
    }

    // Track active leaf membership by packed key for deterministic removal.
    std::unordered_map<std::uint64_t, int> leafIndex;
    leafIndex.reserve(leaves.size());
    for (int i = 0; i < static_cast<int>(leaves.size()); i++)
    {
        leafIndex[PackKey(leaves[static_cast<size_t>(i)].key)] = i;
    }

    while (!heap.empty() && static_cast<int>(leaves.size()) < maxKeys)
    {
        const HeapItem item = heap.top();
        heap.pop();

        const std::uint64_t packed = PackKey(item.cand.key);
        auto it = leafIndex.find(packed);
        if (it == leafIndex.end())
        {
            continue; // already refined
        }

        const Candidate parent = item.cand;
        if (!shouldSplit(parent))
        {
            continue;
        }

        // Remove parent leaf.
        const int removeIndex = it->second;
        const int lastIndex = static_cast<int>(leaves.size()) - 1;
        if (removeIndex != lastIndex)
        {
            leaves[static_cast<size_t>(removeIndex)] = leaves.back();
            leafIndex[PackKey(leaves[static_cast<size_t>(removeIndex)].key)] = removeIndex;
        }
        leaves.pop_back();
        leafIndex.erase(it);

        // Add children leaves.
        const int childLod = parent.key.lod + 1;
        const int cx = parent.key.x * 2;
        const int cy = parent.key.y * 2;

        const Node children[4] = {
            Node{.lod = childLod, .x = cx + 0, .y = cy + 0},
            Node{.lod = childLod, .x = cx + 1, .y = cy + 0},
            Node{.lod = childLod, .x = cx + 0, .y = cy + 1},
            Node{.lod = childLod, .x = cx + 1, .y = cy + 1},
        };

        for (const Node &ch : children)
        {
            if (static_cast<int>(leaves.size()) >= maxKeys)
            {
                break;
            }

            Candidate c{};
            if (!computeCandidate(ch, c))
            {
                continue;
            }

            leafIndex[PackKey(c.key)] = static_cast<int>(leaves.size());
            leaves.push_back(c);
            pushSplitCandidate(c);
        }
    }

    std::sort(leaves.begin(), leaves.end(),
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
                  if (a.key.lod != b.key.lod)
                  {
                      return a.key.lod < b.key.lod;
                  }
                  if (a.key.y != b.key.y)
                  {
                      return a.key.y < b.key.y;
                  }
                  return a.key.x < b.key.x;
              });

    outKeys.reserve(leaves.size());
    for (const Candidate &c : leaves)
    {
        outKeys.push_back(c.key);
    }

    m_lastStats.visibleCandidates = static_cast<int>(leaves.size());
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

    GLint previousActiveTexture = GL_TEXTURE0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glActiveTexture(GL_TEXTURE0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    std::vector<std::uint16_t> heights;
    heights.reserve(static_cast<size_t>(m_config.heightResolution * m_config.heightResolution));

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

        glBindTexture(GL_TEXTURE_2D_ARRAY, m_tileTextureArray);
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

        GenerateTileHeightR16(key, heights);

        glBindTexture(GL_TEXTURE_2D_ARRAY, m_heightTextureArray);
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                        0,
                        0,
                        0,
                        slot,
                        m_config.heightResolution,
                        m_config.heightResolution,
                        1,
                        GL_RED,
                        GL_UNSIGNED_SHORT,
                        heights.data());

        if (slot >= 0 && slot < static_cast<int>(m_slots.size()))
        {
            m_slots[slot].uploaded = true;
        }

        m_lastStats.uploadsThisFrame++;
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glActiveTexture(previousActiveTexture);
}

void TiledBackgroundRenderer::EnsureDrawResources()
{
    if (m_vao == 0)
    {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_instanceVbo);

        glBindVertexArray(m_vao);

        // Vertex buffer is populated below (mesh build).
        // Vertex format:
        // - localXZ (vec2): unit tile coordinates in XZ
        // - uvSkirtEdge (vec4): (u,v,skirtFlag,edgeId)
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

        // location 0: local pos (x,z)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *)0);
        glEnableVertexAttribArray(0);

        // location 1: (u,v,skirtFlag,edgeId)
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *)(sizeof(float) * 2));
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

        glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(TileInstance), (void *)offsetof(TileInstance, skirtMask));
        glEnableVertexAttribArray(6);
        glVertexAttribDivisor(6, 1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        m_instanceCapacity = 1;
    }

    if (!m_meshDirty)
    {
        return;
    }

    // Generate a tessellated unit tile mesh in XZ plus optional skirts.
    // Higher resolution improves the smoothness of displaced (mountain) regions.
    const int seg = std::max(1, m_config.meshResolution);
    const float invSeg = 1.0f / static_cast<float>(seg);

    std::vector<float> verts;
    verts.reserve(static_cast<size_t>(seg * seg * 6 * 6));

    auto push = [&](float x, float z, float skirtFlag, float edgeId)
    {
        // localXZ
        verts.push_back(x);
        verts.push_back(z);
        // uv
        verts.push_back(x);
        verts.push_back(z);
        // skirtFlag (0=surface/top, 1=skirt bottom)
        verts.push_back(skirtFlag);
        // edgeId (-1=surface, 0=west,1=east,2=south,3=north)
        verts.push_back(edgeId);
    };

    // Skirts are emitted first so the main surface can overwrite them (depth writes are disabled for this pass).
    // They are enabled per-tile via iSkirtMask; disabled skirts collapse into degenerate triangles.
    const float surface = 0.0f;
    const float skirt = 1.0f;
    const float west = 0.0f;
    const float east = 1.0f;
    const float south = 2.0f;
    const float north = 3.0f;

    for (int i = 0; i < seg; i++)
    {
        const float t0 = static_cast<float>(i) * invSeg;
        const float t1 = static_cast<float>(i + 1) * invSeg;

        // West edge (x=0): z in [t0..t1]
        push(0.0f, t0, surface, west);
        push(0.0f, t1, surface, west);
        push(0.0f, t1, skirt, west);
        push(0.0f, t0, surface, west);
        push(0.0f, t1, skirt, west);
        push(0.0f, t0, skirt, west);

        // East edge (x=1)
        push(1.0f, t0, surface, east);
        push(1.0f, t1, surface, east);
        push(1.0f, t1, skirt, east);
        push(1.0f, t0, surface, east);
        push(1.0f, t1, skirt, east);
        push(1.0f, t0, skirt, east);

        // South edge (z=0): x in [t0..t1]
        push(t0, 0.0f, surface, south);
        push(t1, 0.0f, surface, south);
        push(t1, 0.0f, skirt, south);
        push(t0, 0.0f, surface, south);
        push(t1, 0.0f, skirt, south);
        push(t0, 0.0f, skirt, south);

        // North edge (z=1)
        push(t0, 1.0f, surface, north);
        push(t1, 1.0f, surface, north);
        push(t1, 1.0f, skirt, north);
        push(t0, 1.0f, surface, north);
        push(t1, 1.0f, skirt, north);
        push(t0, 1.0f, skirt, north);
    }

    for (int y = 0; y < seg; y++)
    {
        const float z0 = static_cast<float>(y) * invSeg;
        const float z1 = static_cast<float>(y + 1) * invSeg;
        for (int x = 0; x < seg; x++)
        {
            const float x0 = static_cast<float>(x) * invSeg;
            const float x1 = static_cast<float>(x + 1) * invSeg;

            // Tri 1
            push(x0, z0, 0.0f, -1.0f);
            push(x1, z0, 0.0f, -1.0f);
            push(x1, z1, 0.0f, -1.0f);
            // Tri 2
            push(x0, z0, 0.0f, -1.0f);
            push(x1, z1, 0.0f, -1.0f);
            push(x0, z1, 0.0f, -1.0f);
        }
    }

    m_tileVertexCount = static_cast<int>(verts.size() / 6);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * verts.size(), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_meshDirty = false;
}

void TiledBackgroundRenderer::DrawTiles(Camera *camera, const std::vector<TileInstance> &instances) const
{
    if (instances.empty() || !m_shader || m_vao == 0 || m_tileVertexCount <= 0)
    {
        return;
    }

    m_shader->Use();

    const glm::vec3 cameraPos = camera->transform.Pos;
    const glm::mat4 view = glm::lookAt(cameraPos, cameraPos + camera->front, camera->up);

    glm::vec3 forward = camera->front;
    forward.y = 0.0f;
    float len = glm::length(forward);
    if (len < 0.0001f)
    {
        forward = glm::vec3(0.0f, 0.0f, -1.0f);
    }
    else
    {
        forward /= len;
    }

    m_shader->SetMat4("view", view);
    m_shader->SetMat4("projection", camera->Projection);
    m_shader->SetFloat("u_planeY", m_config.planeY);
    m_shader->SetVec4("u_fallbackColor", m_config.baseColorA);
    m_shader->SetFloat("u_gridSpacing", m_config.gridSpacing);
    m_shader->SetInt("u_majorEvery", m_config.majorEvery);
    m_shader->SetFloat("u_minorLineWidth", m_config.minorLineWidth);
    m_shader->SetFloat("u_majorLineWidth", m_config.majorLineWidth);
    m_shader->SetVec4("u_minorLineColor", m_config.minorLineColor);
    m_shader->SetVec4("u_majorLineColor", m_config.majorLineColor);
    m_shader->SetFloat("u_farDistance", m_config.farDistance);
    m_shader->SetFloat("u_mountainExtraDistance", m_config.mountainExtraDistance);
    m_shader->SetFloat("u_mountainHeight", m_config.mountainHeight);
    m_shader->SetFloat("u_mountainFadeDistance", m_config.mountainFadeDistance);
    m_shader->SetFloat("u_mountainRiseExponent", m_config.mountainRiseExponent);
    m_shader->SetFloat("u_skirtDepth", m_config.skirtDepth);
    m_shader->SetFloat("u_mountainGridScale", m_config.mountainGridScale);
    m_shader->SetFloat("u_mountainMajorStrength", m_config.mountainMajorStrength);
    m_shader->SetFloat("u_horizonLinePixels", m_config.horizonLinePixels);
    m_shader->SetVec2("u_cameraPosXZ", glm::vec2(cameraPos.x, cameraPos.z));
    m_shader->SetVec2("u_cameraForwardXZ", glm::vec2(forward.x, forward.z));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_tileTextureArray);
    m_shader->SetInt("u_tiles", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_heightTextureArray);
    m_shader->SetInt("u_heights", 1);

    glBindVertexArray(m_vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, m_tileVertexCount, static_cast<GLsizei>(instances.size()));
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glActiveTexture(GL_TEXTURE0);
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
    EnsureDrawResources();

    m_frameIndex++;
    m_lastStats = Stats{};
    m_lastStats.frameIndex = m_frameIndex;

    std::vector<TileKey> visibleKeys;
    SelectVisibleTiles(camera, visibleKeys);

    std::unordered_set<std::uint64_t> keySet;
    keySet.reserve(visibleKeys.size() * 2);
    for (const TileKey &k : visibleKeys)
    {
        keySet.insert(PackKey(k));
    }

    auto containsKey = [&](int lod, int x, int y) -> bool
    {
        return keySet.find(PackKey(TileKey{lod, x, y})) != keySet.end();
    };

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

        // Skirts: enable only on edges where this (coarser) tile borders finer tiles.
        // This hides T-junction cracks at LOD boundaries without adding visible walls between same-LOD neighbors.
        int skirtMask = 0;
        const int lod = key.lod;
        if (lod + 1 < m_config.lodCount)
        {
            const int finerLod = lod + 1;
            const int fx0 = key.x * 2;
            const int fy0 = key.y * 2;

            // West (-X)
            if (!containsKey(lod, key.x - 1, key.y))
            {
                if (containsKey(finerLod, fx0 - 1, fy0) || containsKey(finerLod, fx0 - 1, fy0 + 1))
                {
                    skirtMask |= (1 << 0);
                }
            }

            // East (+X)
            if (!containsKey(lod, key.x + 1, key.y))
            {
                if (containsKey(finerLod, fx0 + 2, fy0) || containsKey(finerLod, fx0 + 2, fy0 + 1))
                {
                    skirtMask |= (1 << 1);
                }
            }

            // South (-Z)
            if (!containsKey(lod, key.x, key.y - 1))
            {
                if (containsKey(finerLod, fx0, fy0 - 1) || containsKey(finerLod, fx0 + 1, fy0 - 1))
                {
                    skirtMask |= (1 << 2);
                }
            }

            // North (+Z)
            if (!containsKey(lod, key.x, key.y + 1))
            {
                if (containsKey(finerLod, fx0, fy0 + 2) || containsKey(finerLod, fx0 + 1, fy0 + 2))
                {
                    skirtMask |= (1 << 3);
                }
            }
        }

        const float tileSize = GetTileWorldSizeForLOD(key.lod);
        const glm::vec2 originXZ(key.x * tileSize, key.y * tileSize);
        instances.push_back(TileInstance{
            .originXZ = originXZ,
            .layer = static_cast<float>(slot),
            .hasData = 0.0f,
            .tileWorldSize = tileSize,
            .skirtMask = static_cast<float>(skirtMask)});
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

    const std::uint64_t logIntervalFrames = 600;
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

    const float z0 = key.y * tileSize;

    for (int y = 0; y < h; y++)
    {
        const float v = (h == 1) ? 0.0f : (static_cast<float>(y) / static_cast<float>(h - 1));
        const float worldZ = z0 + v * tileSize;

        for (int x = 0; x < w; x++)
        {
            // Base gradient: dark -> brighter with distance along -Z (tuned for Tetris camera).
            const float gradT = Clamp01((-worldZ) * 0.0015f);
            const glm::vec4 color = glm::mix(m_config.baseColorA, m_config.baseColorB, gradT);

            const int idx = (y * w + x) * 4;
            outPixels[static_cast<size_t>(idx + 0)] = static_cast<std::uint8_t>(std::clamp(color.r, 0.0f, 1.0f) * 255.0f);
            outPixels[static_cast<size_t>(idx + 1)] = static_cast<std::uint8_t>(std::clamp(color.g, 0.0f, 1.0f) * 255.0f);
            outPixels[static_cast<size_t>(idx + 2)] = static_cast<std::uint8_t>(std::clamp(color.b, 0.0f, 1.0f) * 255.0f);
            outPixels[static_cast<size_t>(idx + 3)] = static_cast<std::uint8_t>(std::clamp(color.a, 0.0f, 1.0f) * 255.0f);
        }
    }
}

void TiledBackgroundRenderer::GenerateTileHeightR16(const TileKey &key, std::vector<std::uint16_t> &outPixels) const
{
    const int w = m_config.heightResolution;
    const int h = m_config.heightResolution;

    outPixels.resize(static_cast<size_t>(w * h));

    // If noiseScale is zero, treat as flat.
    const float scale = (m_config.mountainFeatureSize > 0.0f) ? (1.0f / m_config.mountainFeatureSize) : m_config.mountainNoiseScale;
    if (scale <= 0.0f)
    {
        std::fill(outPixels.begin(), outPixels.end(), 0);
        return;
    }

    const float tileSize = GetTileWorldSizeForLOD(key.lod);
    const float x0 = static_cast<float>(key.x) * tileSize;
    const float z0 = static_cast<float>(key.y) * tileSize;

    const float ruggedness = std::clamp(m_config.mountainDetail, 0.0f, 1.0f);

    for (int y = 0; y < h; y++)
    {
        const float v = (h == 1) ? 0.0f : (static_cast<float>(y) / static_cast<float>(h - 1));
        const float worldZ = z0 + v * tileSize;

        for (int x = 0; x < w; x++)
        {
            const float u = (w == 1) ? 0.0f : (static_cast<float>(x) / static_cast<float>(w - 1));
            const float worldX = x0 + u * tileSize;

            const float nx = worldX * scale;
            const float nz = worldZ * scale;

            // Mountain "shape" is driven by two components:
            // - a low-frequency range mask (broad massing)
            // - a high-frequency ridge field (peaks / crags)
            //
            // mountainDetail (ruggedness) blends between smooth hills (0) and sharp ridges (1).
            // As ruggedness increases, we also increase ridge frequency so distant silhouettes read less like a smooth band.
            const float rangeMask = std::pow(Clamp01(Fbm2D(nx * 0.25f, nz * 0.25f, 3)), 1.35f);

            const float warpAmp = ruggedness * 0.75f;
            const float wx = (Fbm2D(nx * 0.50f + 10.1f, nz * 0.50f + 32.7f, 2) - 0.5f) * warpAmp;
            const float wz = (Fbm2D(nx * 0.50f + 91.7f, nz * 0.50f + 7.3f, 2) - 0.5f) * warpAmp;

            const float smooth = Fbm2D(nx + wx, nz + wz, 4);

            const float ridgeFreq = Lerp(1.0f, 4.0f, ruggedness);
            const float ridged = RidgedFbm2D((nx + wx) * ridgeFreq + 12.3f, (nz + wz) * ridgeFreq + 4.7f, 5);

            float n = Lerp(smooth, ridged, ruggedness);
            n *= (0.35f + 0.65f * rangeMask);

            // Final shaping: emphasize peaks while keeping valleys near the horizon flatter.
            float height01 = Clamp01(std::pow(Clamp01(n), 1.85f));

            // Optional composition: bias towards two dominant ranges (left/right) with a center valley.
            // This is applied in world space so it stays stable per tile key.
            if (m_config.mountainSideStrength > 0.0f)
            {
                const float strength = std::clamp(m_config.mountainSideStrength, 0.0f, 1.0f);
                const float valleyHalfWidth = std::max(0.0f, m_config.mountainSideOffsetX);
                const float falloffWidth = std::max(0.0001f, m_config.mountainSideWidthX);
                const float base = std::clamp(m_config.mountainSideBase, 0.0f, 1.0f);

                // World-X valley mask:
                // - Inside |X| <= valleyHalfWidth, reduce amplitude to "base"
                // - Over falloffWidth, smoothly transition to full amplitude (1.0)
                //
                // This yields two broad mountain masses (left/right) that are visible near the horizon, without requiring
                // huge offsets that may land outside the camera frustum at u_farDistance.
                const float absX = std::abs(worldX);

                // Use a "soft edge" around the valley boundary:
                // - When falloffWidth >= valleyHalfWidth, the valley becomes a smooth bowl (no flat trench).
                // - When falloffWidth < valleyHalfWidth, there is a flatter center band before ramping up.
                const float start = std::max(0.0f, valleyHalfWidth - falloffWidth);
                const float end = valleyHalfWidth + falloffWidth;
                const float denom = std::max(end - start, 0.0001f);
                const float uMask = Clamp01((absX - start) / denom);
                const float t = Fade(uMask); // smoothstep(0..1)

                const float mask = Lerp(base, 1.0f, t);
                height01 *= Lerp(1.0f, mask, strength);
                height01 = Clamp01(height01);
            }

            outPixels[static_cast<size_t>(y * w + x)] = static_cast<std::uint16_t>(std::lround(height01 * 65535.0f));
        }
    }
}

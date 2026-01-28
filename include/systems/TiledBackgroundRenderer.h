/**
 * @file TiledBackgroundRenderer.h
 * @brief World-centric tiled background renderer (procedural tiles + GPU cache)
 * @lines ~220
 *
 * Quick-stats (Public API):
 * - Initialize() - Create GL resources (line ~70)
 * - Shutdown() - Destroy GL resources (line ~85)
 * - Configure() - Update renderer config (line ~95)
 * - SetEnabled() - Toggle rendering (line ~105)
 * - Render() - Select tiles, upload budget, draw instanced quads (line ~120)
 *
 * Purpose:
 * - Implements the "render tiles + GPU cache" half of docs/TERRAIN_IMPLEMENTATION.md,
 *   but with procedural CPU tile generation for a vaporwave-style background.
 *
 * Notes:
 * - Camera-centric clipmaps are explicitly avoided: tiles are keyed by stable (lod, x, y).
 * - Current implementation targets a single LOD (lod=0) as a milestone-0 foundation.
 */

#pragma once

#include "Camera.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Shader;

/**
 * @brief Configuration for tiled background renderer
 */
struct TiledBackgroundConfig
{
    bool enabled = false;     ///< Master toggle (renderer no-ops when disabled)
    float planeY = 0.0f;      ///< World Y coordinate for the ground plane
    float farDistance = 600.0f; ///< How far forward to cover (world units)
    float widthMultiplier = 1.2f; ///< Expands computed view width for safety

    int tileResolution = 256; ///< Tile texture size (pixels, square)
    int cacheSlots = 96;      ///< Max tiles resident on GPU (texture array layers)
    int maxUploadsPerFrame = 2; ///< Budget for new tile uploads per frame

    float tileWorldSize = 64.0f; ///< World units covered by one tile edge

    // Vaporwave grid parameters (world-space)
    float gridSpacing = 4.0f;    ///< Minor grid spacing (world units)
    int majorEvery = 8;          ///< Major line every N minor lines
    float lineWidth = 0.08f;     ///< Line half-width (world units)

    glm::vec4 baseColorA = glm::vec4(0.08f, 0.02f, 0.12f, 1.0f); ///< Near color
    glm::vec4 baseColorB = glm::vec4(0.20f, 0.02f, 0.25f, 1.0f); ///< Far color
    glm::vec4 minorLineColor = glm::vec4(0.0f, 0.85f, 1.0f, 1.0f); ///< Cyan
    glm::vec4 majorLineColor = glm::vec4(1.0f, 0.0f, 0.80f, 1.0f); ///< Magenta
};

/**
 * @brief Renderer that draws a ground plane using cached procedural tiles
 */
class TiledBackgroundRenderer
{
public:
    /**
     * @brief Get singleton instance
     */
    static TiledBackgroundRenderer &GetInstance()
    {
        static TiledBackgroundRenderer instance;
        return instance;
    }

    TiledBackgroundRenderer(TiledBackgroundRenderer const &) = delete;
    void operator=(TiledBackgroundRenderer const &) = delete;

    ~TiledBackgroundRenderer();

    /**
     * @brief Initialize GL resources (safe to call multiple times)
     * @return true if initialized successfully
     */
    bool Initialize();

    /**
     * @brief Destroy GL resources
     */
    void Shutdown();

    /**
     * @brief Configure renderer (recreates GPU cache if needed)
     * @param config New configuration
     */
    void Configure(const TiledBackgroundConfig &config);

    /**
     * @brief Enable/disable rendering
     */
    void SetEnabled(bool enabled);

    /**
     * @brief Get whether rendering is enabled
     */
    bool IsEnabled() const { return m_config.enabled; }

    /**
     * @brief Render tiled background (no-op if disabled/uninitialized)
     * @param camera Active camera
     * @param deltaMs Frame delta in milliseconds (for future animation hooks)
     */
    void Render(Camera *camera, float deltaMs);

private:
    TiledBackgroundRenderer() = default;

    struct TileKey
    {
        int lod = 0;
        int x = 0;
        int y = 0;

        bool operator==(const TileKey &other) const
        {
            return lod == other.lod && x == other.x && y == other.y;
        }
    };

    struct TileInstance
    {
        glm::vec2 originXZ = glm::vec2(0.0f); ///< world-space origin (x,z)
        float layer = 0.0f;                   ///< texture array layer index (float for attrib)
    };

    bool EnsureInitialized();
    void RecreateCacheIfNeeded();

    void SelectVisibleTiles(Camera *camera, std::vector<TileKey> &outKeys) const;
    int ResolveTileLayer(const TileKey &key, bool &outIsNew);

    void UploadPendingTiles();
    void GenerateTileRGBA8(const TileKey &key, std::vector<std::uint8_t> &outPixels) const;

    void EnsureDrawResources();
    void DrawTiles(Camera *camera, const std::vector<TileInstance> &instances) const;

    struct TileSlot
    {
        bool occupied = false;
        TileKey key{};
        std::uint64_t lastUsedFrame = 0;
        bool uploaded = false;
    };

    TiledBackgroundConfig m_config{};

    bool m_initialized = false;
    bool m_cacheDirty = true;

    std::uint64_t m_frameIndex = 0;
    std::uint32_t m_tileTextureArray = 0;

    std::vector<TileSlot> m_slots;
    std::unordered_map<std::uint64_t, int> m_keyToSlot; ///< packed key -> slot index
    std::vector<TileKey> m_pendingUploads;

    // Draw resources
    std::uint32_t m_vao = 0;
    std::uint32_t m_vbo = 0;
    std::uint32_t m_instanceVbo = 0;
    int m_instanceCapacity = 0;
    std::unique_ptr<Shader> m_shader;

    static std::uint64_t PackKey(const TileKey &key);
};

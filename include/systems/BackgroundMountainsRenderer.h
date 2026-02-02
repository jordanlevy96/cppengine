/**
 * @file BackgroundMountainsRenderer.h
 * @brief Procedural mountain mesh with wire grid overlay (background terrain)
 * @lines ~200
 *
 * Quick-stats (Public API):
 * - Initialize() - Create shader + mesh buffers (line ~80)
 * - Shutdown() - Destroy GL resources (line ~95)
 * - Configure() - Update mountain parameters (line ~110)
 * - SetEnabled() - Toggle rendering (line ~125)
 * - Render() - Draw mountain mesh behind ground grid (line ~140)
 *
 * Notes:
 * - Renders after SkyBackground and before TiledBackground.
 * - Mountains are camera-oriented but procedurally generated (static when scrollSpeed=0).
 */

#pragma once

#include "Camera.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>

class Shader;

/**
 * @brief Configuration for background mountain renderer
 */
struct BackgroundMountainsConfig
{
    bool enabled = true; ///< Master toggle

    // Placement (camera-forward)
    float startDistance = 150.0f; ///< Distance along camera forward where mountains start (matches TiledBackground farDistance)
    float depth = 260.0f;         ///< How far mountains extend beyond startDistance (world units)
    float widthMultiplier = 1.8f; ///< Expand computed width from camera FOV (safety/coverage)

    // Height shaping
    float baseY = 0.0f;         ///< Base elevation (world Y)
    float height = 26.0f;       ///< Peak height above baseY (world units)
    float noiseScale = 0.012f;  ///< Noise frequency in world units (higher = more peaks)
    float detail = 0.55f;       ///< Secondary noise strength [0..1]
    float scrollSpeed = 0.0f;   ///< Horizontal scroll speed (world units/sec); 0 = static

    // Grid overlay (world-space on the surface)
    float gridSpacing = 6.0f;     ///< Minor grid spacing (world units)
    int majorEvery = 6;           ///< Every Nth minor line becomes major
    float minorLineWidth = 0.06f; ///< Minor half-width (world units)
    float majorLineWidth = 0.12f; ///< Major half-width (world units)

    glm::vec4 fillColor = glm::vec4(0.04f, 0.01f, 0.06f, 1.0f);      ///< Mountain fill
    glm::vec4 minorLineColor = glm::vec4(0.0f, 0.65f, 0.95f, 1.0f);  ///< Cyan (dim)
    glm::vec4 majorLineColor = glm::vec4(1.0f, 0.0f, 0.80f, 1.0f);   ///< Magenta
};

/**
 * @brief Background mountain renderer (procedural heightfield mesh)
 */
class BackgroundMountainsRenderer
{
public:
    static BackgroundMountainsRenderer &GetInstance()
    {
        static BackgroundMountainsRenderer instance;
        return instance;
    }

    BackgroundMountainsRenderer(BackgroundMountainsRenderer const &) = delete;
    void operator=(BackgroundMountainsRenderer const &) = delete;

    ~BackgroundMountainsRenderer();

    bool Initialize();
    void Shutdown();

    void Configure(const BackgroundMountainsConfig &config);
    void SetEnabled(bool enabled);
    bool IsEnabled() const { return m_config.enabled; }

    void Render(Camera *camera, float deltaMs);

private:
    BackgroundMountainsRenderer() = default;

    bool EnsureInitialized();
    void EnsureMeshResources();
    void Draw(Camera *camera) const;

    BackgroundMountainsConfig m_config{};
    bool m_initialized = false;

    std::uint32_t m_vao = 0;
    std::uint32_t m_vbo = 0;
    int m_vertexCount = 0;

    std::unique_ptr<Shader> m_shader;

    float m_timeSeconds = 0.0f;
};


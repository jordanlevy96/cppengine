/**
 * @file SkyBackgroundRenderer.h
 * @brief Fullscreen sky gradient + sun background pass
 * @lines ~210
 *
 * Quick-stats (Public API):
 * - Initialize() - Create shader and draw resources (line ~70)
 * - Shutdown() - Destroy GL resources (line ~90)
 * - Configure() - Update sky parameters (line ~105)
 * - SetEnabled() - Toggle rendering (line ~120)
 * - Render() - Draw fullscreen sky behind scene (line ~135)
 *
 * Notes:
 * - Renders a fullscreen background (no depth writes) before the tiled ground.
 * - Intended for Milestone 2 in .context/tasks/tiled_background.md.
 */

#pragma once

#include "Camera.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>

class Shader;

/**
 * @brief Configuration for sky background renderer
 */
struct SkyBackgroundConfig
{
    bool enabled = false; ///< Master toggle (renderer no-ops when disabled)

    glm::vec3 topColor = glm::vec3(0.12f, 0.65f, 0.90f);    ///< Gradient top color (linear)
    glm::vec3 bottomColor = glm::vec3(0.02f, 0.02f, 0.08f); ///< Gradient bottom color (linear)

    float horizonY = 0.40f;     ///< Horizon position in [0..1] (0=bottom, 1=top)
    float horizonGlow = 0.08f;  ///< Horizon glow width (higher = wider)
    glm::vec3 horizonColor = glm::vec3(0.95f, 0.10f, 0.80f); ///< Horizon tint (linear)

    glm::vec2 sunPos = glm::vec2(0.72f, 0.62f); ///< Sun center in [0..1] UV space
    float sunRadius = 0.06f;     ///< Sun disc radius in UV space
    float sunGlow = 0.14f;       ///< Sun glow radius multiplier
    glm::vec3 sunColor = glm::vec3(1.00f, 0.55f, 0.15f); ///< Sun tint (linear)
};

/**
 * @brief Fullscreen sky background renderer
 */
class SkyBackgroundRenderer
{
public:
    /**
     * @brief Get singleton instance
     */
    static SkyBackgroundRenderer &GetInstance()
    {
        static SkyBackgroundRenderer instance;
        return instance;
    }

    SkyBackgroundRenderer(SkyBackgroundRenderer const &) = delete;
    void operator=(SkyBackgroundRenderer const &) = delete;

    ~SkyBackgroundRenderer();

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
     * @brief Configure renderer
     * @param config New configuration
     */
    void Configure(const SkyBackgroundConfig &config);

    /**
     * @brief Enable/disable rendering
     */
    void SetEnabled(bool enabled);

    /**
     * @brief Get whether rendering is enabled
     */
    bool IsEnabled() const { return m_config.enabled; }

    /**
     * @brief Get current configuration snapshot
     */
    const SkyBackgroundConfig &GetConfig() const { return m_config; }

    /**
     * @brief Render fullscreen sky (no-op if disabled/uninitialized)
     * @param camera Active camera (unused for now; kept for future fog/sky coupling)
     * @param deltaMs Frame delta in milliseconds
     */
    void Render(Camera *camera, float deltaMs);

private:
    SkyBackgroundRenderer() = default;

    bool EnsureInitialized();
    void EnsureDrawResources();
    void DrawFullscreen() const;

    SkyBackgroundConfig m_config{};
    bool m_initialized = false;

    std::uint32_t m_vao = 0;
    std::unique_ptr<Shader> m_shader;
};

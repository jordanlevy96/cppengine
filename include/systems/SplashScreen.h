/**
 * @file SplashScreen.h
 * @brief Static splash screen displayed during engine initialization
 * @lines ~45
 *
 * Quick-stats (Public API):
 * - Initialize() - Load PNG and create GL resources (line ~30)
 * - Render() - Draw fullscreen textured quad (line ~35)
 * - Shutdown() - Release GL resources (line ~40)
 * - IsReady() - Check if splash loaded successfully (line ~42)
 *
 * Not a singleton. Created during init, destroyed after transition.
 * Uses Composite.shader and stb_image, same patterns as HTMLRendererMT.
 */

#pragma once

#include <string>

/**
 * @brief Fullscreen splash screen rendered during engine boot
 *
 * Loads a PNG texture and draws it as a fullscreen quad using
 * the Composite shader. Designed to be shown immediately after
 * GL context creation and dismissed once the UI renderer is ready.
 */
class SplashScreen
{
public:
    /**
     * @brief Load splash PNG and create GL resources
     * @param pngPath Path to splash PNG file
     * @param viewportWidth Framebuffer width in pixels
     * @param viewportHeight Framebuffer height in pixels
     * @return true if loaded successfully
     */
    bool Initialize(const std::string &pngPath, int viewportWidth, int viewportHeight);

    /**
     * @brief Render splash fullscreen (clears screen, draws quad, no swap)
     */
    void Render();

    /**
     * @brief Release GL resources (texture, VAO, VBO, shader)
     */
    void Shutdown();

    /**
     * @brief Check if splash loaded and is ready to render
     * @return true if Initialize() succeeded
     */
    bool IsReady() const { return m_ready; }

private:
    unsigned int m_texture = 0;     ///< GL texture from splash PNG
    unsigned int m_quadVAO = 0;     ///< Fullscreen quad VAO
    unsigned int m_quadVBO = 0;     ///< Fullscreen quad VBO
    unsigned int m_shaderProgram = 0; ///< Composite shader program (owned)
    int m_width = 0;                ///< Viewport width
    int m_height = 0;               ///< Viewport height
    bool m_ready = false;           ///< True after successful Initialize()
};

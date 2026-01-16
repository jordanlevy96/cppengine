/**
 * @file SceneViewport.h
 * @brief Embedded 3D scene viewport for editor
 * 
 * Renders game scene to offscreen framebuffer (FBO) for display in editor UI.
 */

#pragma once

#include "Camera.h"
#include "controllers/Registry.h"

#include <glad/glad.h>
#include <string>
#include <vector>

/**
 * @brief Viewport that renders scene to texture for editor display
 * 
 * Creates a framebuffer object (FBO) and renders the game scene to it.
 * The resulting texture can be embedded in the editor HTML UI.
 * 
 * **Thread safety**: Must be used on main thread (OpenGL context owner)
 */
class SceneViewport {
public:
    SceneViewport() = default;
    ~SceneViewport();

    /**
     * @brief Initialize FBO and use provided camera
     * @param width Viewport width in pixels
     * @param height Viewport height in pixels
     * @param camera Pointer to camera (owned by EngineCore)
     * @return true if initialization succeeded
     */
    bool Initialize(int width, int height, Camera *camera);

    /**
     * @brief Set which entity to highlight in the viewport
     * @param entityId Entity to highlight (or ENTITY_NULL for no highlight)
     */
    void SetSelectedEntity(EntityID entityId) { m_selectedEntityId = entityId; }

    /**
     * @brief Render scene to FBO texture
     * @note Binds FBO, clears, renders all entities, unbinds FBO
     */
    void Render();

    /**
     * @brief Shutdown and cleanup OpenGL resources
     */
    void Shutdown();

    /**
     * @brief Get rendered color texture as base64 data URI
     * @return String in format "data:image/png;base64,..."
     * @note Expensive operation (~5-10ms), reads from GPU and encodes PNG
     */
    std::string GetTextureAsDataURI();

    /**
     * @brief Get OpenGL texture ID for color attachment
     * @return Texture ID, or 0 if not initialized
     */
    GLuint GetColorTexture() const { return m_colorTexture; }

    /**
     * @brief Get camera reference
     * @return Reference to camera used for viewport rendering
     */
    Camera& GetCamera() { return *m_camera; }

    /**
     * @brief Resize viewport (recreates FBO)
     * @param width New width in pixels
     * @param height New height in pixels
     */
    void Resize(int width, int height);

    /**
     * @brief Get viewport width
     */
    int GetWidth() const { return m_width; }

    /**
     * @brief Get viewport height
     */
    int GetHeight() const { return m_height; }

private:
    /**
     * @brief Create FBO with color and depth attachments
     */
    bool CreateFramebuffer();

    /**
     * @brief Delete FBO resources
     */
    void DeleteFramebuffer();

    /**
     * @brief Encode pixel buffer as PNG, then base64
     * @param pixels RGBA pixel data
     * @param width Image width
     * @param height Image height
     * @return Base64-encoded PNG string
     */
    std::string EncodePNGBase64(const std::vector<uint8_t>& pixels, int width, int height);

    int m_width = 0;
    int m_height = 0;
    bool m_initialized = false;

    // OpenGL resources
    GLuint m_framebuffer = 0;       ///< Framebuffer object
    GLuint m_colorTexture = 0;      ///< Color attachment (RGBA8)
    GLuint m_depthBuffer = 0;       ///< Depth renderbuffer (DEPTH_COMPONENT24)

    // Camera reference (owned by EngineCore)
    Camera *m_camera = nullptr;

    // Registry reference (shared with game)
    Registry* m_registry = nullptr;

    // Selection state
    EntityID m_selectedEntityId = ENTITY_NULL;  ///< Entity to highlight in viewport
};

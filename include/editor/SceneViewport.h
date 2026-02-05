/**
 * @file SceneViewport.h
 * @brief Embedded 3D scene viewport for editor
 * 
 * Renders game scene into a sub-rectangle of the main framebuffer for editor UI.
 */

#pragma once

#include "Camera.h"
#include "controllers/Registry.h"

#include <glad/glad.h>

class Shader;

/**
 * @brief Viewport that renders scene to texture for editor display
 * 
 * Renders the game scene into a specified framebuffer rectangle and supports picking.
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
     * @brief Render scene to the default framebuffer within a rectangle
     * @param fbX Framebuffer X (origin bottom-left)
     * @param fbY Framebuffer Y (origin bottom-left)
     * @param fbWidth Width in pixels
     * @param fbHeight Height in pixels
     */
    void RenderToScreen(int fbX, int fbY, int fbWidth, int fbHeight);

    /**
     * @brief Shutdown and cleanup OpenGL resources
     */
    void Shutdown();

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

    /**
     * @brief Pick an entity at viewport pixel coordinates
     * @param x X coordinate in viewport pixels (0..width-1), origin top-left
     * @param y Y coordinate in viewport pixels (0..height-1), origin top-left
     * @return EntityID if found, otherwise ENTITY_NULL
     * @note Uses an offscreen ID render pass into a pick buffer
     */
    EntityID PickEntityAt(int x, int y);

private:
    /**
     * @brief Create picking framebuffer (ID render pass)
     */
    bool CreatePickFramebuffer();

    /**
     * @brief Delete framebuffer resources
     */
    void DeleteFramebuffer();

    /**
     * @brief Render picking pass into pick framebuffer
     * @note Must be called on main thread with OpenGL context current
     */
    void RenderPickingPass();

    int m_width = 0;
    int m_height = 0;
    bool m_initialized = false;

    // Picking resources
    GLuint m_pickFramebuffer = 0;   ///< Offscreen framebuffer for ID rendering
    GLuint m_pickTexture = 0;       ///< Color attachment storing encoded EntityID
    GLuint m_pickDepthBuffer = 0;   ///< Depth for picking pass
    Shader *m_pickShader = nullptr; ///< Flat shader used to render IDs

    // Camera reference (owned by EngineCore)
    Camera *m_camera = nullptr;

    // Registry reference (shared with game)
    Registry* m_registry = nullptr;

    // Selection state
    EntityID m_selectedEntityId = ENTITY_NULL;  ///< Entity to highlight in viewport
};

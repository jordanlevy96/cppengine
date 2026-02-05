/**
 * @file Camera.h
 * @brief 3D perspective camera with FPS-style controls
 */

#pragma once

#include "components/Transform.h"

#include <glad/glad.h>

#include <vector>

/// Camera movement directions for FPS-style controls
enum CameraDirections
{
    FORWARD,    ///< Move forward in look direction
    BACK,       ///< Move backward from look direction
    LEFT,       ///< Strafe left
    RIGHT       ///< Strafe right
};

/**
 * @brief 3D perspective camera with transform and FPS controls
 *
 * Supports mouse-look rotation and WASD-style movement.
 * Uses standard OpenGL perspective projection.
 */
class Camera
{
public:
    float fov = 45.0f;                                  ///< Field of view in degrees
    float moveSpeed = 0.05f;                             ///< Movement speed multiplier (world units per second)
    Transform transform;                                ///< Camera position and orientation
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);   ///< Forward direction vector
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);       ///< Up direction vector
    glm::mat4 Projection = glm::mat4(1.0f);            ///< Projection matrix

    /**
     * @brief Construct camera with perspective projection
     * @param width Viewport width in pixels
     * @param height Viewport height in pixels
     */
    Camera(float width, float height)
    {
        SetPerspective(fov, width, height);
        transform.Pos = glm::vec3(0.0f, 0.0f, -10.0f);
    };
    ~Camera(){};

    /**
     * @brief Update perspective with current FOV and last known viewport size
     * @param fov Field of view in degrees
     */
    void SetPerspective(float fov);

    /**
     * @brief Update perspective projection matrix
     * @param fov Field of view in degrees
     * @param width Viewport width in pixels
     * @param height Viewport height in pixels
     */
    void SetPerspective(float fov, float width, float height);

    /**
     * @brief Translate camera by offset
     * @param translate Translation vector in world space
     */
    void Translate(glm::vec3 translate);

    /**
     * @brief Move camera in specified direction (FPS controls)
     * @param dir Movement direction (FORWARD, BACK, LEFT, RIGHT)
     * @param deltaTime Time delta for frame-rate independent movement
     */
    void Move(CameraDirections dir, float deltaTime);

    /**
     * @brief Update camera rotation based on mouse position (mouse-look)
     * @param xpos Mouse X position in screen coordinates
     * @param ypos Mouse Y position in screen coordinates
     */
    void RotateByMouse(double xpos, double ypos);

    /**
     * @brief Set yaw/pitch rotation and update forward vector
     * @param yawDegrees Yaw angle in degrees
     * @param pitchDegrees Pitch angle in degrees
     */
    void SetYawPitch(float yawDegrees, float pitchDegrees);

private:
    bool firstMouse = true;             ///< Flag to prevent jump on first mouse input

    float lastWidth = 0, lastHeight = 0;    ///< Last viewport dimensions for perspective updates

    float yaw = 90.0f;                  ///< Yaw angle (left/right rotation)
    float pitch = 0.0f;                 ///< Pitch angle (up/down rotation)
    float lastX = 0.0f, lastY = 0.0f;   ///< Last mouse position for delta calculation
};

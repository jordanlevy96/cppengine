#pragma once

#include "components/Transform.h"

#include <glm/glm.hpp>
#include <glad/glad.h>

#include <vector>

enum CameraDirections
{
    FORWARD,
    BACK,
    LEFT,
    RIGHT
};

class Camera
{
public:
    float fov = 45.0f;
    Transform transform;
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 Projection = glm::mat4(1.0f);

    Camera(float width, float height)
    {
        SetPerspective(fov, width, height);
        transform.Pos = glm::vec3(0.0f, 0.0f, -10.0f);
    };
    ~Camera(){};
    void SetPerspective(float fov);
    void SetPerspective(float fov, float width, float height);
    void Translate(glm::vec3 translate);
    void Move(CameraDirections dir, float deltaTime);
    void RotateByMouse(double xpos, double ypos);

private:
    bool firstMouse = true;

    float lastWidth, lastHeight = 0;

    float yaw = 90.0f;
    float pitch = 0.0f;
    float lastX, lastY = 0.0f;
};

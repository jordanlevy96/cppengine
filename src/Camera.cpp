#include "Camera.h"

#include <algorithm>
#include <iostream>

void Camera::SetPerspective(float fov)
{
    SetPerspective(fov, lastWidth, lastHeight);
}

void Camera::SetPerspective(float fovInDegrees, float width, float height)
{
    fov = fovInDegrees;
    lastWidth = width;
    lastHeight = height;
    // Far plane of 10000 units to support terrain viewing
    Projection = glm::perspective(glm::radians(fov), width / height, 0.1f, 10000.0f);
}

void Camera::RotateByMouse(double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = -cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = -sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(direction);
}

void Camera::SetYawPitch(float yawDegrees, float pitchDegrees)
{
    yaw = yawDegrees;
    pitch = std::clamp(pitchDegrees, -89.0f, 89.0f);

    glm::vec3 direction;
    direction.x = -cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = -sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(direction);

    // Prevent cursor jumps on next mouse movement.
    firstMouse = true;
}

void Camera::Move(CameraDirections dir, float deltaTime)
{
    float speed = moveSpeed * deltaTime;
    switch (dir)
    {
    case (CameraDirections::FORWARD):
        transform.Pos += speed * front;
        break;
    case (CameraDirections::BACK):
        transform.Pos -= speed * front;
        break;
    case (CameraDirections::LEFT):
        transform.Pos -= glm::normalize(glm::cross(front, up)) * speed;
        break;
    case (CameraDirections::RIGHT):
        transform.Pos += glm::normalize(glm::cross(front, up)) * speed;
        break;
    default:
        std::cerr << "Invalid direction given to camera" << std::endl;
    }
}

void Camera::Translate(glm::vec3 translate)
{
    transform.Pos += translate;
}

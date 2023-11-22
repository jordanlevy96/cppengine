#include "entities/Camera.h"

#include <iostream>

void Camera::SetPerspective(float fovInDegrees, float width, float height)
{
    fov = fovInDegrees;
    Projection = glm::perspective(glm::radians(fov), width / height, 0.1f, 100.0f); // TODO: settings for near and far
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
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(direction);
}

void Camera::Move(CameraDirections dir, float deltaTime)
{
    float speed = 0.05f * deltaTime;
    switch (dir)
    {
    case (CameraDirections::FORWARD):
        transform.pos += speed * front;
        break;
    case (CameraDirections::BACK):
        transform.pos -= speed * front;
        break;
    case (CameraDirections::LEFT):
        transform.pos -= glm::normalize(glm::cross(front, up)) * speed;
        break;
    case (CameraDirections::RIGHT):
        transform.pos += glm::normalize(glm::cross(front, up)) * speed;
        break;
    default:
        std::cerr << "Invalid direction given to camera" << std::endl;
    }
}

void Camera::Translate(glm::vec3 translate)
{
    transform.pos += translate;
}
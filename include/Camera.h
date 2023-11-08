#pragma once

#include <GameObject3D.h>

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
    glm::mat4 process(double frametime);
    glm::mat4 View = glm::mat4(1.0f);
    glm::mat4 Projection = glm::mat4(1.0f);
    Camera(float width, float height, glm::vec3 start)
    {
        lastX = width / 2;
        lastY = height / 2;
        pos = start;
        // View = glm::translate(View, pos);
    };
    ~Camera(){};
    void SetPerspective(float degrees, float width, float height);
    void Render(GameObject *obj);
    void RenderAll(std::vector<GameObject *> objects);
    void Translate(glm::vec3 translate);
    void Move(CameraDirections dir, float deltaTime);
    void RotateByMouse(double xpos, double ypos);

private:
    glm::vec3 pos;
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    bool firstMouse = true;

    float yaw = 90.0f;
    float pitch = 0.0f;
    float lastX, lastY;
    float fov = 45.0f;
};

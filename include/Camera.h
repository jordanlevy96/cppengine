#pragma once

#include <GameObject.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

#include <vector>

class Camera
{
public:
    glm::mat4 process(double frametime);
    glm::mat4 View = glm::mat4(1.0f);
    glm::mat4 Projection = glm::mat4(1.0f);
    Camera(glm::vec3 start)
    {
        View = glm::translate(View, start);
    };
    ~Camera(){};
    void SetPerspective(float degrees, float width, float height);
    void Render(GameObject *obj);
    void RenderAll(std::vector<GameObject *> objects);
};

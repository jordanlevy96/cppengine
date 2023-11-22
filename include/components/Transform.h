#pragma once

#include "components/Component.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace EulerAngles
{
    const glm::vec3 Pitch(0.0f, 0.0f, 1.0f);
    const glm::vec3 Roll(1.0f, 0.0f, 0.0f);
    const glm::vec3 Yaw(0.0f, 1.0f, 0.0f);
};

class Transform : Component
{
public:
    glm::vec3 pos = glm::vec3(1.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec3 eulers = glm::vec3(0.0f);

    Transform(){};
    ~Transform(){};

    void Translate(glm::vec3 translate);
    void Scale(glm::vec3 scale);
    void Rotate(float degrees, glm::vec3 dir);
    glm::mat4 GetMatrix() const;

    const std::string GetName() override
    {
        return TRANFORM;
    };

private:
    glm::mat4 transform = glm::mat4(1.0f);
};
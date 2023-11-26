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

struct Transform : public Component
{
    glm::vec3 Pos = glm::vec3(1.0f);
    glm::vec3 Scale = glm::vec3(1.0f);
    glm::vec3 Eulers = glm::vec3(0.0f);
    glm::vec3 Color = glm::vec3(1.0f);
    glm::mat4 GetMatrix() const
    {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), Pos);
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), Eulers.x, EulerAngles::Pitch) *
                             glm::rotate(glm::mat4(1.0f), Eulers.y, EulerAngles::Yaw) *
                             glm::rotate(glm::mat4(1.0f), Eulers.z, EulerAngles::Roll);
        glm::mat4 scaling = glm::scale(glm::mat4(1.0f), Scale);

        return translation * rotation * scaling;
    }

    void Translate(glm::vec3 translate)
    {
        Pos += translate;
    }

    void Rotate(float degrees, glm::vec3 dir)
    {
        Eulers += degrees * dir;
    }

    ComponentTypes GetType() const override
    {
        return ComponentTypes::TransformType;
    };
};
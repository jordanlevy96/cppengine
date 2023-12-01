#pragma once

#include "components/Component.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp> // For glm::toMat4

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
    glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 Color = glm::vec3(1.0f);
    glm::mat4 GetMatrix() const
    {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), Pos);
        glm::mat4 rotation = glm::toMat4(Rotation);
        glm::mat4 scaling = glm::scale(glm::mat4(1.0f), Scale);

        return translation * rotation * scaling;
    }

    void Translate(glm::vec3 translate)
    {
        Pos += translate;
    }

    void Rotate(float angle, glm::vec3 axis)
    {
        glm::quat deltaRotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis));
        Rotation = glm::normalize(deltaRotation * Rotation);
    }

    ComponentTypes GetType() const override
    {
        return ComponentTypes::TransformType;
    };
};
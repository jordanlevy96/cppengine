#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>

#define EntityID size_t

namespace EulerAngles
{
    const glm::vec3 Pitch(0.0f, 0.0f, 1.0f);
    const glm::vec3 Roll(1.0f, 0.0f, 0.0f);
    const glm::vec3 Yaw(0.0f, 1.0f, 0.0f);
};

struct Transform
{
    glm::vec3 Pos = glm::vec3(0.0f);
    glm::vec3 Scale = glm::vec3(1.0f);
    glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 Color = glm::vec3(1.0f);

    EntityID Parent = -1;
    std::vector<EntityID> Children;

    void AddChild(EntityID id)
    {
        Children.push_back(id);
    }
};
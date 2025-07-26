#pragma once

#include "components/Transform.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp> // For glm::toMat4

typedef size_t EntityID;

namespace TransformUtils
{
    glm::mat4 calculateMatrix(Transform transform);
    void translate(EntityID entity, const glm::vec3 &translation);
    void move_to(EntityID entity, const glm::vec3 &newPos);
    void rotate(EntityID entity, float angle, const glm::vec3 &axis);
};
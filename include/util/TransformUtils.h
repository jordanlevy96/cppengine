#pragma once

#include "components/Transform.h"
#include "components/CompositeEntity.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp> // For glm::toMat4

namespace TransformUtils
{
    glm::mat4 calculateMatrix(std::shared_ptr<Transform> transform);
    void translate(unsigned int entity, const glm::vec3 &translation);
    void rotate(unsigned int entity, float angle, const glm::vec3 &axis);
};
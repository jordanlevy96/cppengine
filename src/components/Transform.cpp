#include "components/Transform.h"

void Transform::Translate(glm::vec3 translate)
{
    pos += translate;
}

void Transform::Scale(glm::vec3 scale)
{
    this->scale *= scale;
}

void Transform::Rotate(float degrees, glm::vec3 dir)
{
    eulers += degrees * dir;
}

glm::mat4 Transform::GetMatrix() const
{
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), pos);
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), eulers.x, EulerAngles::Pitch) *
                         glm::rotate(glm::mat4(1.0f), eulers.y, EulerAngles::Yaw) *
                         glm::rotate(glm::mat4(1.0f), eulers.z, EulerAngles::Roll);
    glm::mat4 scaling = glm::scale(glm::mat4(1.0f), scale);

    return translation * rotation * scaling;
}

#include "components/Transform.h"

void Transform::Translate(glm::vec3 translate)
{
    Pos += translate;
}

void Transform::Rotate(float degrees, glm::vec3 dir)
{
    Eulers += degrees * dir;
}

glm::mat4 Transform::GetMatrix() const
{
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), Pos);
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), Eulers.x, EulerAngles::Pitch) *
                         glm::rotate(glm::mat4(1.0f), Eulers.y, EulerAngles::Yaw) *
                         glm::rotate(glm::mat4(1.0f), Eulers.z, EulerAngles::Roll);
    glm::mat4 scaling = glm::scale(glm::mat4(1.0f), Scale);

    return translation * rotation * scaling;
}

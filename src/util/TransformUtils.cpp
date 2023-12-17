#include "util/TransformUtils.h"

#include "controllers/Registry.h"

static Registry *registry = &Registry::GetInstance();

void TransformUtils::translate(EntityID entity, const glm::vec3 &translation)
{
    Transform transform = registry->GetComponent<Transform>(entity);
    HierarchyComponent hc = registry->GetComponent<HierarchyComponent>(entity);
    transform.Pos += translation;

    for (EntityID childId : hc.Children)
    {
        TransformUtils::translate(childId, translation);
    }
}

void TransformUtils::move_to(EntityID entity, const glm::vec3 &newPos)
{
    Transform transform = registry->GetComponent<Transform>(entity);
    glm::vec3 difference = newPos - transform.Pos;
    translate(entity, difference);
}

void TransformUtils::rotate(EntityID entity, float angle, const glm::vec3 &axis)
{
    glm::quat deltaRotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis));
    Transform transform = registry->GetComponent<Transform>(entity);
    transform.Rotation = glm::normalize(deltaRotation * transform.Rotation);

    HierarchyComponent hc = registry->GetComponent<HierarchyComponent>(entity);

    for (EntityID childId : hc.Children)
    {
        Transform childTransform = registry->GetComponent<Transform>(childId);

        // Assuming parent's position is the rotation pivot for the entire composite entity
        glm::vec3 relativePos = childTransform.Pos - transform.Pos;
        glm::quat rotatedPos = deltaRotation * glm::quat(0.0f, relativePos.x, relativePos.y, relativePos.z) * glm::conjugate(deltaRotation);
        childTransform.Pos = transform.Pos + glm::vec3(rotatedPos.x, rotatedPos.y, rotatedPos.z);

        TransformUtils::rotate(childId, angle, axis);
    }
}

glm::mat4 TransformUtils::calculateMatrix(Transform transform)
{
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), transform.Pos);
    glm::mat4 rotation = glm::toMat4(transform.Rotation);
    glm::mat4 scaling = glm::scale(glm::mat4(1.0f), transform.Scale);

    return translation * rotation * scaling;
}
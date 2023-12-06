#include "util/TransformUtils.h"

#include "controllers/Registry.h"

static Registry *registry = &Registry::GetInstance();

void TransformUtils::translate(unsigned int entity, const glm::vec3 &translation)
{
    std::shared_ptr<Transform> transform = registry->GetComponent<Transform>(entity);
    if (transform)
    {
        transform->Pos += translation;
    }

    std::shared_ptr<CompositeEntity> composite = registry->GetComponent<CompositeEntity>(entity);
    if (composite)
    {
        for (unsigned int childId : composite->children)
        {
            TransformUtils::translate(childId, translation);
        }
    }
}

void TransformUtils::rotate(unsigned int entity, float angle, const glm::vec3 &axis)
{
    glm::quat deltaRotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis));
    std::shared_ptr<Transform> parentTransform = registry->GetComponent<Transform>(entity);

    if (parentTransform)
    {
        parentTransform->Rotation = glm::normalize(deltaRotation * parentTransform->Rotation);
    }

    std::shared_ptr<CompositeEntity> composite = registry->GetComponent<CompositeEntity>(entity);
    if (composite)
    {
        for (unsigned int childId : composite->children)
        {
            std::shared_ptr<Transform> childTransform = registry->GetComponent<Transform>(childId);
            if (childTransform)
            {
                // Assuming parent's position is the rotation pivot for the entire composite entity
                glm::vec3 relativePos = childTransform->Pos - parentTransform->Pos;
                glm::quat rotatedPos = deltaRotation * glm::quat(0.0f, relativePos.x, relativePos.y, relativePos.z) * glm::conjugate(deltaRotation);
                childTransform->Pos = parentTransform->Pos + glm::vec3(rotatedPos.x, rotatedPos.y, rotatedPos.z);

                TransformUtils::rotate(childId, angle, axis);
            }
        }
    }
}

glm::mat4 TransformUtils::calculateMatrix(std::shared_ptr<Transform> transform)
{
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), transform->Pos);
    glm::mat4 rotation = glm::toMat4(transform->Rotation);
    glm::mat4 scaling = glm::scale(glm::mat4(1.0f), transform->Scale);

    return translation * rotation * scaling;
}
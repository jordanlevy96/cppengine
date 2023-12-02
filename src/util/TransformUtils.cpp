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
}

void TransformUtils::rotate(unsigned int entity, float angle, const glm::vec3 &axis)
{
    std::shared_ptr<Transform> transform = registry->GetComponent<Transform>(entity);
    if (transform)
    {
        glm::quat deltaRotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis));
        transform->Rotation = glm::normalize(deltaRotation * transform->Rotation);
    }

    std::shared_ptr<CompositeEntity> composite = registry->GetComponent<CompositeEntity>(entity);

    if (composite)
    {
        for (unsigned int childId : composite->children)
        {
            std::shared_ptr<Transform> childTransform = registry->GetComponent<Transform>(childId);
            if (childTransform)
            {
                glm::vec3 relativePos = childTransform->Pos - transform->Pos;
                glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, axis);
                glm::vec4 rotatedPos = rotationMatrix * glm::vec4(relativePos, 1.0f);

                childTransform->Pos = glm::vec3(rotatedPos) + transform->Pos;
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
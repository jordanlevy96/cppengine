/**
 * @file HierarchySystem.cpp
 * @brief Implementation of world transform computation from hierarchy
 */

#include "systems/HierarchySystem.h"
#include "controllers/Registry.h"
#include "components/Transform.h"
#include "components/WorldTransform.h"
#include "components/HierarchyComponent.h"
#include "util/TransformUtils.h"

static Registry* registry = &Registry::GetInstance();

void HierarchySystem::Update()
{
    const glm::mat4 identity(1.0f);

    // Find all root entities (Parent == -1) and process their subtrees
    for (EntityID id : registry->GetComponentSet<HierarchyComponent>().GetEntities())
    {
        HierarchyComponent& hc = registry->GetComponent<HierarchyComponent>(id);

        // Root entities have Parent == -1 (wraps to size_t max value)
        if (hc.Parent == static_cast<EntityID>(-1))
        {
            UpdateEntity(id, identity);
        }
    }
}

void HierarchySystem::UpdateEntity(EntityID entity, const glm::mat4& parentWorld)
{
    // Get components
    Transform& local = registry->GetComponent<Transform>(entity);
    WorldTransform& world = registry->GetComponent<WorldTransform>(entity);
    HierarchyComponent& hc = registry->GetComponent<HierarchyComponent>(entity);

    // Compute local matrix (T * R * S)
    glm::mat4 localMatrix = TransformUtils::calculateMatrix(local);

    // Compose with parent: World = ParentWorld * Local
    world.matrix = parentWorld * localMatrix;

    // Recursively process children with our world matrix
    for (EntityID child : hc.Children)
    {
        UpdateEntity(child, world.matrix);
    }
}

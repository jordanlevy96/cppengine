#include "util/SceneTraversal.h"
#include <algorithm>

void AddChild(EntityID parent, EntityID child)
{
    Registry* registry = &Registry::GetInstance();
    HierarchyComponent &parentHC = registry->GetComponent<HierarchyComponent>(parent);
    HierarchyComponent &childHC = registry->GetComponent<HierarchyComponent>(child);

    parentHC.Children.push_back(child);
    childHC.Parent = parent;
}

EntityID GetParent(EntityID child)
{
    Registry* registry = &Registry::GetInstance();
    HierarchyComponent childHC = registry->GetComponent<HierarchyComponent>(child);
    return childHC.Parent;
}

void RemoveChild(EntityID parent, EntityID child)
{
    Registry* registry = &Registry::GetInstance();
    HierarchyComponent &parentHC = registry->GetComponent<HierarchyComponent>(parent);
    HierarchyComponent &childHC = registry->GetComponent<HierarchyComponent>(child);

    // Remove child from parent's children list
    auto it = std::find(parentHC.Children.begin(), parentHC.Children.end(), child);
    if (it != parentHC.Children.end())
    {
        parentHC.Children.erase(it);
    }

    // Clear child's parent reference
    childHC.Parent = -1;

    // Convert child's transform from relative to absolute world coordinates
    Transform &childTransform = registry->GetComponent<Transform>(child);
    Transform &parentTransform = registry->GetComponent<Transform>(parent);

    // Child's world position = parent position + child relative position
    childTransform.Pos = parentTransform.Pos + childTransform.Pos;
}

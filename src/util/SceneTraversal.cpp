#include "util/SceneTraversal.h"

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

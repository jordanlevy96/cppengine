#pragma once

#include "controllers/Registry.h"

static Registry *registry = &Registry::GetInstance();

void AddChild(EntityID parent, EntityID child)
{
    HierarchyComponent parentHC = registry->GetComponent<HierarchyComponent>(parent);
    HierarchyComponent childHC = registry->GetComponent<HierarchyComponent>(child);

    parentHC.Children.push_back(child);
    childHC.Parent = parent;
}

EntityID GetParent(EntityID child)
{
    HierarchyComponent childHC = registry->GetComponent<HierarchyComponent>(child);
    return childHC.Parent;
}

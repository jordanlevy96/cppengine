#pragma once

#include <vector>

typedef size_t EntityID;

struct HierarchyComponent
{
    EntityID Parent;
    std::vector<EntityID> Children;

    HierarchyComponent(EntityID parent) : Parent(parent) {}
};

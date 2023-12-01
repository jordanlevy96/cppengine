#pragma once

#include "components/Component.h"
#include "components/Transform.h"

struct CompositeEntity : Component
{
    std::vector<unsigned int> children;
    std::shared_ptr<Transform> transform;

    void AddChild(unsigned int id)
    {
        children.push_back(id);
    }
    ComponentTypes GetType() const override
    {
        return ComponentTypes::CompositeType;
    };
};

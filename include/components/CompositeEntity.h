#pragma once

#include "components/Transform.h"

#include <vector>
#include <memory>

struct CompositeEntity
{
    std::vector<unsigned int> children;
    std::shared_ptr<Transform> transform;

    void AddChild(unsigned int id)
    {
        children.push_back(id);
    }
};

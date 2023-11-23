#pragma once

#include "util/globals.h"

#include <string>

enum ComponentTypes
{
    BaseComponentType = -1,
    TransformType,
    RenderComponentType,
    LightingType,
    EmitterType
};

class Component
{
public:
    Component(){};
    virtual ~Component(){};
    virtual const std::string GetName() = 0;
};

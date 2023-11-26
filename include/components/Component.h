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

struct Component
{
    virtual ComponentTypes GetType() const { return BaseComponentType; }
};

#pragma once

#include "components/RenderComponent.h"
#include "components/Transform.h"

struct Lighting
{
    Transform *LightTrans; // maybe change this to the Light's ID?

    Lighting(Transform *lightTransform) : LightTrans(lightTransform) {}
};

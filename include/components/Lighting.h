#pragma once

#include "components/RenderComponent.h"
#include "components/Transform.h"

struct Lighting
{
    EntityID LightID;

    Lighting(EntityID lightID) : LightID(lightID) {}
};

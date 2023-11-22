#pragma once

#include "entities/Camera.h"
#include "components/RenderComponent.h"

class RenderSystem
{
    static void RenderEntity(unsigned int id, Camera cam);
};
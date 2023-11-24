#pragma once

#include "Camera.h"
#include "components/RenderComponent.h"

class RenderSystem
{
public:
    static void RenderEntity(unsigned int id, Camera *cam);
};
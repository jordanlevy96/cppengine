#pragma once

#include "controllers/Registry.h"

class TweenSystem
{
public:
    static void Update(float delta);

private:
    static void UpdateTween(EntityID id, float delta);
};
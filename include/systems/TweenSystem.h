#pragma once

#include "controllers/Registry.h"
#include "util/debug.h"

class TweenSystem
{
public:
    static void Update(float delta);

private:
    static void UpdateTween(std::pair<unsigned int, std::shared_ptr<Tween>> pair, float delta);
};
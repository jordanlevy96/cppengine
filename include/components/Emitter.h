#pragma once

#include "components/RenderComponent.h"

class Emitter : public RenderComponent
{
public:
    Emitter(const std::string &shaderSrc, const std::string &meshSrc) : RenderComponent(shaderSrc, meshSrc){};
    const std::string GetName() override
    {
        return EMITTER_COMPONENT;
    }
};

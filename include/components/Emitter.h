#pragma once

#include "components/RenderComponent.h"

struct Emitter : public RenderComponent
{
    Emitter(const std::string &shaderSrc, const std::string &meshSrc) : RenderComponent(shaderSrc, meshSrc){};

    ComponentTypes GetType() const override
    {
        return ComponentTypes::EmitterType;
    }
};

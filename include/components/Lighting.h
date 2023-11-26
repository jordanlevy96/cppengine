#pragma once

#include "components/RenderComponent.h"
#include "components/Transform.h"

struct Lighting : public Component
{
    std::shared_ptr<RenderComponent> RC;
    std::shared_ptr<Transform> LightTrans;
    glm::vec3 *ObjectColor;

    Lighting(const std::shared_ptr<RenderComponent> &rc, glm::vec3 *objColor, const std::shared_ptr<Transform> &lightTransformPointer)
        : RC(rc), ObjectColor(objColor), LightTrans(lightTransformPointer) {}

    ComponentTypes GetType() const override
    {
        return ComponentTypes::LightingType;
    }
};

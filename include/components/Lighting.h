#pragma once

#include "components/RenderComponent.h"
#include "components/Transform.h"

class Lighting : public RenderComponent
{
public:
    Lighting(const std::string &shaderSrc, const std::string &meshSrc, glm::vec3 *objColor, Transform *lightTransformPointer) : RenderComponent(shaderSrc, meshSrc), objectColor(objColor), lightTrans(lightTransformPointer){};
    void SetUniforms() override;

private:
    Transform *lightTrans;
    glm::vec3 *objectColor;
};

#pragma once

#include "Camera.h"
#include "components/RenderComponent.h"

#include "util/debug.h"

class RenderSystem
{
public:
    static void RenderEntity(unsigned int id, Camera *cam);
    static void SetUniforms(std::unordered_map<UniformWrapper, Uniform> uniforms, Shader *shader);
};
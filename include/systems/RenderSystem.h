#pragma once

#include "Camera.h"
#include "components/RenderComponent.h"
#include "components/Tween.h"

class RenderSystem
{
public:
    static void Update(Camera *cam, float delta);

private:
    template <typename T>
    static void RenderEntity(EntityID id, Camera *cam);
    static void SetUniforms(std::unordered_map<UniformWrapper, Uniform> uniforms, Shader *shader);
};
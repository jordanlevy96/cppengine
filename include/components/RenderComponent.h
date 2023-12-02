#pragma once

#include "controllers/ResourceManager.h"
#include "components/Component.h"
#include "util/Uniform.h"

#include <memory>

struct RenderComponent : public Component
{
    std::shared_ptr<Shader> shader;
    std::shared_ptr<Mesh> mesh;
    std::unordered_map<UniformWrapper, Uniform> uniforms;

    RenderComponent(const std::string &shaderSrc, const std::string &meshSrc)
    {
        shader = ResourceManager::GetInstance().GetShader(shaderSrc);
        mesh = ResourceManager::GetInstance().GetMesh(meshSrc);
    }
    void AddUniform(std::string name, Uniform u, UniformTypeMap type)
    {
        UniformWrapper uw = {name, type};
        uniforms[uw] = u;
    }
};

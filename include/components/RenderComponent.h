#pragma once

#include "components/Component.h"
#include "components/Transform.h"
#include "util/Mesh.h"
#include "util/Shader.h"
#include "util/Uniform.h"

#include <memory>

static std::unordered_map<std::string, std::shared_ptr<Shader>> shaders;
static std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;

struct RenderComponent : public Component
{
    std::shared_ptr<Shader> shader;
    std::shared_ptr<Mesh> mesh;
    std::unordered_map<UniformWrapper, Uniform> uniforms;

    RenderComponent(const std::string &shaderSrc, const std::string &meshSrc)
    {
        if (shaders.find(shaderSrc) == shaders.end())
        {
            shaders[shaderSrc] = std::make_shared<Shader>(shaderSrc);
        }
        shader = shaders[shaderSrc];

        if (meshes.find(meshSrc) == meshes.end())
        {
            meshes[meshSrc] = std::make_shared<Mesh>(meshSrc);
        }
        mesh = meshes[meshSrc];
    }
    ComponentTypes GetType() const override
    {
        return ComponentTypes::RenderComponentType;
    }
    void AddUniform(std::string name, Uniform u, UniformTypeMap type)
    {
        UniformWrapper uw = {name, type};
        uniforms[uw] = u;
    }
};

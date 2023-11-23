#pragma once

#include "components/Component.h"
#include "components/Transform.h"
#include "util/Mesh.h"
#include "util/Shader.h"
#include "util/Uniform.h"

class RenderComponent : public Component
{
public:
    std::shared_ptr<Shader> shader;
    std::shared_ptr<Mesh> mesh;

    RenderComponent(const std::string &shaderSrc, const std::string &meshSrc);

    void AddUniform(std::string name, Uniform u, UniformTypeMap type);
    virtual void SetUniforms(std::shared_ptr<Transform> transform);
    const std::string GetName() override
    {
        return RENDER_COMPONENT;
    };

protected:
    static std::unordered_map<std::string, std::shared_ptr<Shader>> shaders;
    static std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
    std::unordered_map<UniformWrapper, Uniform> uniforms;
};
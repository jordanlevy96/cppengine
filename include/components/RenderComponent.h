#pragma once

#include "components/Component.h"
#include "util/Mesh.h"
#include "util/Shader.h"
#include "util/Uniform.h"

class RenderComponent : public Component
{
public:
    Mesh *mesh;
    Shader *shader;

    RenderComponent(const std::string &shaderSrc, const std::string &meshSrc);

    void AddUniform(std::string name, Uniform u, UniformTypeMap type);
    virtual void SetUniforms();
    const std::string GetName() override
    {
        return RENDER_COMPONENT;
    };

protected:
    static std::unordered_map<std::string, Shader *> shaders;
    static std::unordered_map<std::string, Mesh *> meshes;
    std::unordered_map<UniformWrapper, Uniform> uniforms;
};
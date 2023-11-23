#include "components/RenderComponent.h"

#include <iostream>

std::unordered_map<std::string, std::shared_ptr<Shader>> RenderComponent::shaders;
std::unordered_map<std::string, std::shared_ptr<Mesh>> RenderComponent::meshes;

RenderComponent::RenderComponent(const std::string &shaderSrc, const std::string &meshSrc)
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

void RenderComponent::AddUniform(std::string name, Uniform u, UniformTypeMap type)
{
    UniformWrapper uw = {name, type};
    uniforms[uw] = u;
}

void RenderComponent::SetUniforms(std::shared_ptr<Transform> transform)
{
    shader->setMat4("model", transform->GetMatrix());
    for (std::pair<UniformWrapper, Uniform> pair : uniforms)
    {
        std::string name = pair.first.name;
        UniformTypeMap type = pair.first.type;
        Uniform u = pair.second;

        switch (type)
        {
        case UniformTypeMap::b:
            shader->setBool(name, std::get<bool>(u));
            break;
        case UniformTypeMap::i:
            shader->setInt(name, std::get<int>(u));
            break;
        case UniformTypeMap::f:
            shader->setFloat(name, std::get<float>(u));
            break;
        case UniformTypeMap::vec2:
            shader->setVec2(name, std::get<glm::vec2>(u));
            break;
        case UniformTypeMap::vec3:
            shader->setVec3(name, std::get<glm::vec3>(u));
            break;
        case UniformTypeMap::vec4:
            shader->setVec4(name, std::get<glm::vec4>(u));
            break;
        case UniformTypeMap::mat2:
            shader->setMat2(name, std::get<glm::mat2>(u));
            break;
        case UniformTypeMap::mat3:
            shader->setMat3(name, std::get<glm::mat3>(u));
            break;
        case UniformTypeMap::mat4:
            shader->setMat4(name, std::get<glm::mat4>(u));
            break;
        default:
            std::cerr << "Invalid Uniform Type!" << std::endl;
            break;
        }
    }
}

#include <GameObject.h>
#include <Shader.h>
#include <Mesh.h>

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

GameObject::GameObject(const char *shaderSrcFile, const char *meshSrcFile)
{
    transform = glm::mat4(1.0f);
    shader = new Shader(shaderSrcFile);
    mesh = new Mesh(meshSrcFile);
}

GameObject::~GameObject()
{
    delete shader;
    delete mesh;
}

void GameObject::Transform(float radians, glm::vec3 scale, glm::vec3 translate)
{
    transform = glm::translate(transform, translate);
    transform = glm::rotate(transform, radians, glm::vec3(0.0, 0.0, 1.0));
    transform = glm::scale(transform, scale);
}

void GameObject::Translate(glm::vec3 translate)
{
    transform = glm::translate(transform, translate);
}

void GameObject::Scale(glm::vec3 scale)
{
    transform = glm::scale(transform, scale);
}

void GameObject::AddTexture(const char *textureSrcPath, bool alpha)
{
    mesh->AddTexture(textureSrcPath, alpha);
}

void GameObject::Update(float deltaTime)
{
}

void GameObject::AddUniform(std::string name, Uniform u, UniformTypeMap type)
{
    UniformWrapper uw = {name, type};
    uniforms[uw] = u;
}

void GameObject::SetUniforms()
{
    shader->setMat4("model", transform);
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

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

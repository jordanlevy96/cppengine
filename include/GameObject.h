#pragma once

#include <Shader.h>
#include <Mesh.h>
#include <Uniform.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

class GameObject
{
public:
    GameObject(const char *shaderSrc, const char *meshSrc);
    virtual ~GameObject();

    virtual void Update(float deltaTime);

    void Render();
    void AddTexture(const char *textureSrcPath, bool alpha);
    void Transform(float radians, glm::vec3 scale, glm::vec3 translate);

    void Translate(glm::vec3 translate);
    void Scale(glm::vec3 scale);

    void AddUniform(std::string name, Uniform u, UniformTypeMap type);
    void SetUniforms();

    Mesh *mesh;
    Shader *shader;
    std::vector<unsigned int> textures;
    glm::mat4 transform;

private:
    std::unordered_map<UniformWrapper, Uniform> uniforms;
};
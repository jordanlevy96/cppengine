#pragma once

#include <Shader.h>
#include <Uniform.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL // allows vec3 in unordered_map
#include "glm/gtx/hash.hpp"

#include <tiny_obj_loader.h>

class Mesh
{
public:
    Mesh(const char *modelSrc);
    ~Mesh();
    void AddTexture(const char *textureSrc, bool alpha);
    void AddUniform(std::string name, Uniform u, UniformTypeMap type);
    void SetUniforms(glm::mat4 transform, Shader *shader);
    unsigned int VAO, EBO;
    std::vector<unsigned int> textures;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

private:
    unsigned int VBO;
    std::unordered_map<UniformWrapper, UniformTypeMap> uniforms;
};

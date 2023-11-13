#pragma once

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
    unsigned int VAO, EBO;
    std::vector<unsigned int> textures;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

private:
    unsigned int VBO;
};

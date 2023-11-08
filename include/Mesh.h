#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"

#include <tiny_obj_loader.h>

class Mesh
{
public:
    Mesh(const char *modelSrc);
    ~Mesh();
    void AddTexture(const char *textureSrc, bool alpha);
    unsigned int VAO;
    std::vector<unsigned int> textures;
    std::vector<float> vertices;

private:
    unsigned int VBO;
};

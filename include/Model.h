#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"

#include <tiny_obj_loader.h>

class Model
{
public:
    Model(const char *modelSrc);
    ~Model();
    void AddTexture(const char *textureSrc, bool alpha);
    unsigned int VAO;
    std::vector<unsigned int> textures;
    // std::vector<unsigned int> indices;
    std::vector<float> vertices;
    int numFaces = 0;

private:
    // void ExtractIndices(std::vector<tinyobj::shape_t> shapes, tinyobj::attrib_t attrib);
    // void InterleaveVertices(tinyobj::attrib_t attrib);
    unsigned int VBO;
};

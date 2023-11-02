#pragma once

#include <glm/glm.hpp>
#include <tiny_obj_loader.h>

struct Indices
{
    std::vector<int> Vertex;
    std::vector<int> Normal;
    std::vector<int> Texture;
};

class Model
{
public:
    Model(const char *modelSrc);
    ~Model();
    void AddTexture(const char *textureSrc, bool alpha);
    unsigned int VAO;
    int numVertices;
    std::vector<unsigned int> textures;

private:
    void ExtractIndices(std::vector<tinyobj::shape_t> shapes);
    unsigned int VBO, EBO;
    Indices extractedIndices;
};

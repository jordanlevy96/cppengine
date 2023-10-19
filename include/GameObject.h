#pragma once
#include <glm/glm.hpp>
#include <vector>

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Color;
    glm::vec2 TexCoords;
};

class GameObject
{
public:
    // Constructor
    GameObject(float x, float y, float width, float height, float *vertices, int numVertices, unsigned int *indices, int numIndices, char *shaderSrc);

    // Destructor
    ~GameObject();

    // Update the game object's state
    virtual void Update(float deltaTime);

    // Render the game object
    void Render();

    void AddTexture(const char *textureSrcPath, bool alpha);

protected:
    // Position and size of the game object
    float x;
    float y;
    float width;
    float height;

    std::vector<Vertex> vertices;
    int numVertices;

    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
    unsigned int shader;
    std::vector<unsigned int> textures;
};
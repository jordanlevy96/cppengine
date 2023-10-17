#pragma once

class GameObject
{
public:
    // Constructor
    GameObject(float x, float y, float width, float height, float *vertices, int numVertices, int vertexLength, unsigned int *indices, int numIndices, char *shaderSrc);

    // Destructor
    ~GameObject();

    // Update the game object's state
    virtual void Update(float deltaTime);

    // Render the game object
    void Render();

protected:
    // Position and size of the game object
    float x_;
    float y_;
    float width_;
    float height_;

    float *vertices_;
    int numVertices_;

    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
    unsigned int shader;
};
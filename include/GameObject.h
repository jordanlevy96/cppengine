#pragma once

#include <Shader.h>
#include <Model.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
    GameObject(const char *shaderSrc, const char *modelSrc);
    virtual ~GameObject();

    virtual void Update(float deltaTime);

    void Render();
    void AddTexture(const char *textureSrcPath, bool alpha);
    void Transform(float radians, glm::vec3 scale, glm::vec3 translate);

    void Translate(glm::vec3 translate);
    void Scale(glm::vec3 scale);

    Model *model;
    Shader *shader;
    std::vector<unsigned int> textures;
    glm::mat4 transform;

protected:
    float x;
    float y;
    float width;
    float height;

    std::vector<Vertex> vertices;
    int numVertices;
};
#pragma once

#include <Shader.h>
#include <Mesh.h>

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
    GameObject(const char *shaderSrc, const char *meshSrc);
    virtual ~GameObject();

    void AddTexture(const char *textureSrcPath, bool alpha);
    void Transform(float radians, glm::vec3 scale, glm::vec3 translate);

    void Translate(glm::vec3 translate);
    void Scale(glm::vec3 scale);

    Mesh *mesh;
    Shader *shader;
    std::vector<unsigned int> textures;
    glm::mat4 transform;
};
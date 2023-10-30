#include <GameObject.h>
#include <Shader.h>
#include <stb_image.h>

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

std::vector<Vertex> ParseVertices(const float *data, int vertexCount)
{
    std::vector<Vertex> vertices;
    for (int i = 0; i < vertexCount; i += 8)
    {
        Vertex vertex;
        vertex.Position = glm::vec3(data[i], data[i + 1], data[i + 2]);
        vertex.Color = glm::vec3(data[i + 3], data[i + 4], data[i + 5]);
        vertex.TexCoords = glm::vec2(data[i + 6], data[i + 7]);
        vertices.push_back(vertex);
    }
    return vertices;
}

GameObject::GameObject(float x_, float y_, float width_, float height_, float *vertices_, int numVertices_, unsigned int *indices, int numIndices, char *shaderSrcFile)
    : x(x_), y(y_), width(width_), height(height_), numVertices(numVertices_)
{
    vertices = ParseVertices(vertices_, numVertices);
    transform = glm::mat4(1.0f);
    shader = new Shader(shaderSrcFile);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * numIndices, indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    glEnableVertexAttribArray(0);

    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

GameObject::~GameObject()
{
    std::cout << "destructing game object" << std::endl;
    delete shader;
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    for (GLuint texture : textures)
    {
        glDeleteTextures(1, &texture);
    }
    textures.clear();
}

void GameObject::Transform(float radians, glm::vec3 scale, glm::vec3 translate)
{
    transform = glm::mat4(1.0f);
    transform = glm::translate(transform, translate);
    transform = glm::rotate(transform, radians, glm::vec3(0.0, 0.0, 1.0));
    transform = glm::scale(transform, scale);
}

static GLuint loadTexture(const char *filepath, bool alpha)
{
    GLuint texture;
    int width, height, numChannels;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned char *data = stbi_load(filepath, &width, &height, &numChannels, 0);
    GLenum format = alpha ? GL_RGBA : GL_RGB;
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    return texture;
}

void GameObject::AddTexture(const char *textureSrcPath, bool alpha)
{
    textures.push_back(loadTexture(textureSrcPath, alpha));
}

void GameObject::Update(float deltaTime)
{
}

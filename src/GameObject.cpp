#include <GameObject.h>
#include <Renderer.h>
#include <Texture.h>

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

GameObject::GameObject(float x_, float y_, float width_, float height_, float *vertices_, int numVertices_, unsigned int *indices, int numIndices, char *shaderSrcFile, char *textureSrcPath)
    : x(x_), y(y_), width(width_), height(height_), numVertices(numVertices_)
{
    vertices = ParseVertices(vertices_, numVertices);

    // Init Shader
    ShaderProgramSource source = Renderer::ParseShader(shaderSrcFile);
    unsigned int vertexShader = Renderer::CompileShader(source.VertexSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = Renderer::CompileShader(source.FragmentSource, GL_FRAGMENT_SHADER);
    shader = Renderer::LinkShaders(vertexShader, fragmentShader);

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

    Texture t = Texture(textureSrcPath);
    texture = t.texture;
}

GameObject::~GameObject()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader);
}

void GameObject::Render()
{
    glUseProgram(shader);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(VAO);
    Renderer::Render();
}

void GameObject::Update(float deltaTime)
{
}

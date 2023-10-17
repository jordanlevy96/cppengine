#include <GameObject.h>
#include <Renderer.h>
#include <iostream>

GameObject::GameObject(float x, float y, float width, float height, float *vertices, int numVertices, int vertexLength, unsigned int *indices, int numIndices, char *shaderSrcFile)
    : x_(x), y_(y), width_(width), height_(height), vertices_(vertices), numVertices_(numVertices)
{
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertexLength * numVertices, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * numIndices, indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexLength * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
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
    glBindVertexArray(VAO);
    Renderer::Render();
}

void GameObject::Update(float deltaTime)
{
}

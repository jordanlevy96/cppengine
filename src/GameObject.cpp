#include <GameObject.h>
#include <Renderer.h>
#include <iostream>

GameObject::GameObject(float x, float y, float width, float height, float *vertices, int numVertices, int vertexLength, char *shaderSrcFile)
    : x_(x), y_(y), width_(width), height_(height), vertices_(vertices), numVertices_(numVertices)
{
    for (int i = 0; i < numVertices_; i += 3)
    {
        std::cout << "Vertex " << i / 3 << ": (" << vertices_[i] << ", " << vertices_[i + 1] << ", " << vertices_[i + 2] << ")\n";
    }
    // Constructor code, if any
    std::cout << "Init GameObject" << std::endl;

    ShaderProgramSource source = Renderer::ParseShader(shaderSrcFile);
    unsigned int vertexShader = Renderer::CompileShader(source.VertexSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = Renderer::CompileShader(source.FragmentSource, GL_FRAGMENT_SHADER);
    shader = Renderer::LinkShaders(vertexShader, fragmentShader);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(float) * vertexLength, vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, numVertices * sizeof(float), (void *)0);
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
    Renderer::Render(shader, VAO);
}

void GameObject::Update(float deltaTime)
{
    // Update logic, if any
}

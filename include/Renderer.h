#pragma once
#include <glad/glad.h>
#include <string>

struct ShaderProgramSource
{
    std::string VertexSource;
    std::string FragmentSource;
};

class Renderer
{
public:
    static ShaderProgramSource ParseShader(const std::string &filepath);
    static unsigned int CompileShader(std::string src, int type);
    static unsigned int LinkShaders(unsigned int vertexShader, unsigned int fragmentShader);
    static void Render(unsigned int &shaderProgram, unsigned int &VAO);
};

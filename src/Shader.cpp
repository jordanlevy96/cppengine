#include <Shader.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>

static const int ERR_LOG_SIZE = 512;

static ShaderProgramSource ParseShader(const std::string &filepath)
{
    std::ifstream stream(filepath);
    if (!stream.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    enum class ShaderType
    {
        NONE = -1,
        VERTEX = 0,
        FRAGMENT = 1
    };
    std::string line;
    std::stringstream ss[2];
    ShaderType type = ShaderType::NONE;
    while (getline(stream, line))
    {
        if (line.find("#shader vertex") != std::string::npos)
        {
            type = ShaderType::VERTEX;
        }
        else if (line.find("#shader fragment") != std::string::npos)
        {
            type = ShaderType::FRAGMENT;
        }
        else
        {
            ss[(int)type] << line << '\n';
        }
    }

    if (type == ShaderType::NONE)
    {
        std::cerr << "Error: Shader type not found in " << filepath << std::endl;
    }

    return {ss[0].str(), ss[1].str()};
}

static unsigned int CompileShader(std::string source, int type)
{
    unsigned int shader;
    const char *src = source.c_str();
    shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[ERR_LOG_SIZE];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        // if I ever use other types of shaders, they need to be added in the check here
        const char *shader_type = type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT";
        std::cerr << "ERROR::SHADER::" << shader_type << "::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
        return 0;
    }
    return shader;
}

void Shader::LinkShaders(unsigned int vertexShader, unsigned int fragmentShader)
{
    ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID);

    // check shader linking results
    int success;
    char infoLog[ERR_LOG_SIZE];
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::Shader(const char *filepath)
{
    ShaderProgramSource source = ParseShader(filepath);
    unsigned int vertexShader = CompileShader(source.VertexSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = CompileShader(source.FragmentSource, GL_FRAGMENT_SHADER);
    LinkShaders(vertexShader, fragmentShader);
}

Shader::~Shader()
{
    glDeleteProgram(ID);
}
#include "util/Shader.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>

static const int ERR_LOG_SIZE = 512;

static ShaderProgramSource parseShader(const std::string &filepath)
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

static unsigned int compileShader(std::string source, int type)
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
        // any new shader types need to be handled here and in the parser above
        const char *shader_type = type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT";
        std::cerr << "ERROR::SHADER::" << shader_type << "::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
        return 0;
    }
    return shader;
}

static unsigned int linkShaders(unsigned int vertexShader, unsigned int fragmentShader)
{
    unsigned int id = glCreateProgram();
    glAttachShader(id, vertexShader);
    glAttachShader(id, fragmentShader);
    glLinkProgram(id);

    // error checking
    int success;
    char infoLog[ERR_LOG_SIZE];
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(id, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }

    return id;
}

Shader::Shader(const std::string &filepath)
{
    ShaderProgramSource source = parseShader(filepath);
    unsigned int vertexShader = compileShader(source.VertexSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(source.FragmentSource, GL_FRAGMENT_SHADER);
    ID = linkShaders(vertexShader, fragmentShader);
    // cleanup; the shader data is stored inside the linked object, so these aren't needed anymore
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader()
{
    glDeleteProgram(ID);
}

void Shader::SetUniforms(std::unordered_map<UniformWrapper, Uniform> uniforms)
{
    for (std::pair<UniformWrapper, Uniform> pair : uniforms)
    {
        std::string name = pair.first.name;
        UniformTypeMap type = pair.first.type;
        Uniform u = pair.second;

        switch (type)
        {
        case UniformTypeMap::b:
            SetBool(name, std::get<bool>(u));
            break;
        case UniformTypeMap::i:
            SetInt(name, std::get<int>(u));
            break;
        case UniformTypeMap::f:
            SetFloat(name, std::get<float>(u));
            break;
        case UniformTypeMap::vec2:
            SetVec2(name, std::get<glm::vec2>(u));
            break;
        case UniformTypeMap::vec3:
            SetVec3(name, std::get<glm::vec3>(u));
            break;
        case UniformTypeMap::vec4:
            SetVec4(name, std::get<glm::vec4>(u));
            break;
        case UniformTypeMap::mat2:
            SetMat2(name, std::get<glm::mat2>(u));
            break;
        case UniformTypeMap::mat3:
            SetMat3(name, std::get<glm::mat3>(u));
            break;
        case UniformTypeMap::mat4:
            SetMat4(name, std::get<glm::mat4>(u));
            break;
        default:
            std::cerr << "Invalid Uniform Type!" << std::endl;
            break;
        }
    }
}

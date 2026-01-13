/**
 * @file Shader.h
 * @brief GLSL shader program wrapper with uniform setters
 */

#pragma once

#include "util/Uniform.h"

#include <glm/glm.hpp>
#include <glad/glad.h>

#include <unordered_map>
#include <string>

/**
 * @brief Parsed shader source containing vertex and fragment shaders
 */
struct ShaderProgramSource
{
    std::string VertexSource;      ///< Vertex shader GLSL source code
    std::string FragmentSource;    ///< Fragment shader GLSL source code
};

/**
 * @brief OpenGL shader program wrapper
 *
 * Loads, compiles, and links vertex/fragment shaders from a single file.
 * Shader file format uses `#shader vertex` and `#shader fragment` directives.
 *
 * Example shader file:
 * @code
 * #shader vertex
 * #version 330 core
 * layout(location = 0) in vec3 aPos;
 * // ... vertex shader code ...
 *
 * #shader fragment
 * #version 330 core
 * out vec4 FragColor;
 * // ... fragment shader code ...
 * @endcode
 */
class Shader
{
public:
    unsigned int ID;  ///< OpenGL shader program ID

    /**
     * @brief Load and compile shader from file
     * @param filepath Path to shader file (relative to executable)
     * @throws std::runtime_error if compilation or linking fails
     */
    Shader(const std::string &filepath);

    ~Shader();

    /**
     * @brief Activate this shader for rendering
     */
    void Use()
    {
        glUseProgram(ID);
    }

    /**
     * @brief Set boolean uniform
     * @param name Uniform variable name
     * @param value Boolean value
     */
    void SetBool(const std::string &name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    /**
     * @brief Set integer uniform
     * @param name Uniform variable name
     * @param value Integer value
     */
    void SetInt(const std::string &name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    /**
     * @brief Set float uniform
     * @param name Uniform variable name
     * @param value Float value
     */
    void SetFloat(const std::string &name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    /**
     * @brief Set vec2 uniform
     * @param name Uniform variable name
     * @param value 2D vector
     */
    void SetVec2(const std::string &name, const glm::vec2 &value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    /**
     * @brief Set vec2 uniform from components
     * @param name Uniform variable name
     * @param x X component
     * @param y Y component
     */
    void SetVec2(const std::string &name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }

    /**
     * @brief Set vec3 uniform
     * @param name Uniform variable name
     * @param value 3D vector
     */
    void SetVec3(const std::string &name, const glm::vec3 &value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    /**
     * @brief Set vec3 uniform from components
     * @param name Uniform variable name
     * @param x X component
     * @param y Y component
     * @param z Z component
     */
    void SetVec3(const std::string &name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }

    /**
     * @brief Set vec4 uniform
     * @param name Uniform variable name
     * @param value 4D vector
     */
    void SetVec4(const std::string &name, const glm::vec4 &value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    /**
     * @brief Set vec4 uniform from components
     * @param name Uniform variable name
     * @param x X component
     * @param y Y component
     * @param z Z component
     * @param w W component
     */
    void SetVec4(const std::string &name, float x, float y, float z, float w) const
    {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }

    /**
     * @brief Set 2x2 matrix uniform
     * @param name Uniform variable name
     * @param mat 2x2 matrix
     */
    void SetMat2(const std::string &name, const glm::mat2 &mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    /**
     * @brief Set 3x3 matrix uniform
     * @param name Uniform variable name
     * @param mat 3x3 matrix
     */
    void SetMat3(const std::string &name, const glm::mat3 &mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    /**
     * @brief Set 4x4 matrix uniform
     * @param name Uniform variable name
     * @param mat 4x4 matrix (commonly projection/view/model)
     */
    void SetMat4(const std::string &name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    /**
     * @brief Set multiple uniforms from a map
     * @param uniforms Map of uniform names to values
     */
    void SetUniforms(std::unordered_map<UniformWrapper, Uniform> uniforms);
};

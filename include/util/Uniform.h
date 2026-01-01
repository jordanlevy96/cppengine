/**
 * @file Uniform.h
 * @brief GLSL uniform type-safe wrappers for Shader class
 */

#pragma once

#include <glm/glm.hpp>

#include <variant>
#include <string>

/**
 * @brief Enumeration of supported GLSL uniform types
 *
 * Maps to std::variant types in Uniform alias.
 * Used for runtime type identification in Shader::SetUniforms().
 */
enum UniformTypeMap
{
    b,      ///< bool
    i,      ///< int
    f,      ///< float
    vec2,   ///< glm::vec2
    vec3,   ///< glm::vec3
    vec4,   ///< glm::vec4
    mat2,   ///< glm::mat2
    mat3,   ///< glm::mat3
    mat4    ///< glm::mat4
};

/**
 * @brief Type-safe variant for GLSL uniform values
 *
 * Supports all common GLSL types: scalars, vectors, matrices.
 * Use with Shader::SetUniforms() for batch uniform updates.
 *
 * @see Shader::SetUniforms()
 * @see UniformWrapper
 */
using Uniform = std::variant<bool, int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat2, glm::mat3, glm::mat4>;

/**
 * @brief Uniform identifier with name and type
 *
 * Used as key in unordered_map for Shader::SetUniforms().
 * Hashed by name only (type is metadata).
 *
 * @note Two uniforms with same name but different types will collide
 * @note Developer must ensure unique uniform names per shader
 *
 * Usage example:
 * @code
 * std::unordered_map<UniformWrapper, Uniform> uniforms;
 * uniforms[{"model", UniformTypeMap::mat4}] = glm::mat4(1.0f);
 * uniforms[{"color", UniformTypeMap::vec3}] = glm::vec3(1.0f, 0.5f, 0.0f);
 * shader->SetUniforms(uniforms);
 * @endcode
 */
struct UniformWrapper
{
    std::string name;       ///< Uniform variable name in shader
    UniformTypeMap type;    ///< Type enum (for runtime dispatch)
};

namespace std
{
    /**
     * @brief Hash specialization for UniformWrapper
     *
     * Hashes only the name field (type is ignored).
     * Allows UniformWrapper to be used as unordered_map key.
     */
    template <>
    struct hash<UniformWrapper>
    {
        std::size_t operator()(const UniformWrapper &uw) const
        {
            return std::hash<std::string>()(uw.name);
        }
    };
}

/**
 * @brief Equality operator for UniformWrapper
 * @param lhs Left operand
 * @param rhs Right operand
 * @return True if names are equal (type ignored)
 * @note Uniforms are compared by name only
 * @note Developer must ensure unique names per shader
 */
inline bool operator==(const UniformWrapper &lhs, const UniformWrapper &rhs)
{
    return lhs.name == rhs.name;
}

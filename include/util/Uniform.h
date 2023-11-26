#pragma once

#include <glm/glm.hpp>

#include <variant>
#include <string>

enum UniformTypeMap
{
    b,
    i,
    f,
    vec2,
    vec3,
    vec4,
    mat2,
    mat3,
    mat4
};

using Uniform = std::variant<bool, int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat2, glm::mat3, glm::mat4>;

struct UniformWrapper
{
    std::string name;
    UniformTypeMap type;
};

namespace std
{
    template <>
    struct hash<UniformWrapper>
    {
        std::size_t operator()(const UniformWrapper &uw) const
        {
            return std::hash<std::string>()(uw.name);
        }
    };
}

// Uniforms are compared by their names.
// Two uniforms should never have the same name, and this is up to the developer to enforce.
inline bool operator==(const UniformWrapper &lhs, const UniformWrapper &rhs)
{
    return lhs.name == rhs.name;
}

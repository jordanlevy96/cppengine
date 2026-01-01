/**
 * @file RenderComponent.h
 * @brief Rendering data component for mesh, shader, and material properties
 */

#pragma once

#include "controllers/ResourceManager.h"
#include "util/Uniform.h"

#include <memory>

/**
 * @brief Component defining an entity's visual representation
 *
 * Holds references to mesh geometry, shader program, and custom uniform values.
 * Resources (shader/mesh) are managed by ResourceManager for efficient sharing.
 *
 * **Required for Rendering:**
 * - Transform component (automatically present on all entities)
 * - RenderComponent (this component)
 * - Valid mesh and shader references
 *
 * **Resource Management:**
 * - Shaders and meshes are loaded once and shared via ResourceManager
 * - Multiple entities can reference the same shader/mesh (memory efficient)
 * - Resources are automatically freed when no longer referenced
 *
 * **Uniforms:**
 * Custom shader uniforms can be added per-entity using AddUniform().
 * Common uniforms (model matrix, view, projection) are set by RenderSystem.
 *
 * **Usage:**
 * @code
 * // Create render component with shared resources
 * RenderComponent rc("../res/shaders/Basic.shader", "../res/meshes/cube.obj");
 *
 * // Add custom uniform
 * Uniform roughness;
 * roughness.Float = 0.5f;
 * rc.AddUniform("u_Roughness", roughness, UniformTypeMap::FLOAT);
 *
 * // Register to entity
 * registry.RegisterComponent(entityId, rc);
 * @endcode
 *
 * @note Entities without RenderComponent are not drawn (e.g., lights, logic entities)
 * @see ResourceManager for asset loading
 * @see RenderSystem for rendering pipeline
 */
struct RenderComponent
{
    std::shared_ptr<Shader> shader;  ///< Shader program (shared resource)
    std::shared_ptr<Mesh> mesh;      ///< 3D geometry (shared resource)
    std::unordered_map<UniformWrapper, Uniform> uniforms;  ///< Per-entity shader uniforms

    /**
     * @brief Construct render component and load resources
     * @param shaderSrc Path to shader file (e.g., "../res/shaders/Basic.shader")
     * @param meshSrc Path to mesh file (e.g., "../res/meshes/cube.obj")
     * @note Resources are loaded via ResourceManager (cached and shared)
     */
    RenderComponent(const std::string &shaderSrc, const std::string &meshSrc)
    {
        shader = ResourceManager::GetInstance().GetShader(shaderSrc);
        mesh = ResourceManager::GetInstance().GetMesh(meshSrc);
    }

    /**
     * @brief Add custom shader uniform value
     * @param name Uniform name in shader (e.g., "u_Roughness")
     * @param u Uniform value to set
     * @param type Uniform type (FLOAT, VEC3, MAT4, etc.)
     *
     * @code
     * Uniform metallic;
     * metallic.Float = 0.8f;
     * renderComp.AddUniform("u_Metallic", metallic, UniformTypeMap::FLOAT);
     * @endcode
     */
    void AddUniform(std::string name, Uniform u, UniformTypeMap type)
    {
        UniformWrapper uw = {name, type};
        uniforms[uw] = u;
    }
};

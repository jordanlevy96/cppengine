/**
 * @file RenderSystem.h
 * @brief ECS system for 3D scene rendering with lighting support
 */

#pragma once

#include "Camera.h"
#include "components/RenderComponent.h"
#include "components/Tween.h"

/**
 * @brief Static system for rendering 3D entities with meshes, shaders, and lighting
 *
 * Iterates through entities with RenderComponent and renders them using OpenGL.
 * Supports hierarchical transformations (parent/child relationships) and lighting.
 *
 * **Rendering Pipeline:**
 * 1. Process entities with Lighting component (setup light uniforms)
 * 2. Render entities with RenderComponent (meshes with shaders)
 * 3. Apply camera transformations (view/projection matrices)
 * 4. Handle parent transforms for hierarchical entities
 *
 * **Performance:**
 * - Typical cost: 2-3ms per frame (depends on entity count)
 * - Batching not yet implemented (each entity = 1 draw call)
 *
 * @note All methods are static - this is a stateless system
 * @note Rendering occurs on main thread with active OpenGL context
 */
class RenderSystem
{
public:
    /**
     * @brief Render all entities with RenderComponent or Lighting components
     * @param cam Camera providing view and projection matrices
     * @param delta Time since last frame in seconds (currently unused)
     * @note Must be called from main thread with OpenGL context active
     */
    static void Update(Camera *cam, float delta);

    /**
     * @brief Render a single entity based on component type
     * @tparam T Component type (RenderComponent or Lighting)
     * @param id Entity ID to render
     * @param cam Camera for view/projection calculations
     * @note Template specializations handle different component types
     * @note Made public to allow custom rendering (e.g., selection highlighting in editor)
     */
    template <typename T>
    static void RenderEntity(EntityID id, Camera *cam);

private:

    /**
     * @brief Apply uniform values to active shader program
     * @param uniforms Map of uniform names to values
     * @param shader Shader program to set uniforms on
     * @note Currently unused - uniforms set via Shader::SetUniforms()
     */
    static void SetUniforms(std::unordered_map<UniformWrapper, Uniform> uniforms, Shader *shader);
};
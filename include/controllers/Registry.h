/**
 * @file Registry.h
 * @brief Entity Component System (ECS) registry with SparseSet storage
 * @lines ~345
 *
 * Quick-stats (Public API):
 * - RegisterEntity() - Create new entity with name/parent (line ~65, 70)
 * - DestroyEntity() - Remove entity + children recursively (line ~75)
 * - GetEntityByName() - Find entity by name string (line ~85)
 * - LoadScene() - Load YAML scene definition (line ~100)
 * - AttachScript() - Bind Lua/Python script to entity (line ~115, 120)
 *
 * Based on EnTT library with hierarchical parent/child relationships
 * Wraps entt::registry with game-specific lifecycle management
 * Implementation: See src/controllers/Registry.cpp (345 lines)
 */

#pragma once

#include "util/SparseSet.h"  // EntityID, ENTITY_NULL, SparseSet<T>

#include "components/HierarchyComponent.h"
#include "components/Lighting.h"
#include "components/RenderComponent.h"
#include "components/ScriptComponent.h"
#include "components/Transform.h"
#include "components/Tween.h"
#include "components/WorldTransform.h"

#include <yaml-cpp/yaml.h>

/**
 * @brief Entity Component System registry singleton
 *
 * Manages entity lifecycle and component storage using SparseSet.
 * All entities automatically get a Transform component.
 *
 * **Supported components:**
 * - Transform (position, rotation, scale)
 * - RenderComponent (mesh, shader, material)
 * - ScriptComponent (Lua/Python behaviors)
 * - HierarchyComponent (parent/child relationships)
 * - Tween (animation/interpolation)
 * - Lighting (light properties)
 *
 * **Usage:**
 * @code
 * auto& reg = Registry::GetInstance();
 * EntityID entity = reg.RegisterEntity("player");
 * RenderComponent rc = { ... };
 * reg.RegisterComponent(entity, rc);
 * @endcode
 */
class Registry
{
public:
    /**
     * @brief Get singleton instance
     * @return Reference to Registry singleton
     */
    static Registry &GetInstance()
    {
        static Registry instance;
        return instance;
    }

    /**
     * @brief Cleanup all entities and components
     */
    void Shutdown();

    /**
     * @brief Register new entity with auto-generated name
     * @param parent Parent entity ID (default: none)
     * @return New entity ID
     * @note Automatically creates Transform component
     */
    EntityID RegisterEntity(EntityID parent = -1);

    /**
     * @brief Register new entity with name
     * @param name Entity name for lookup
     * @param parent Parent entity ID (default: none)
     * @return New entity ID
     */
    EntityID RegisterEntity(const std::string &name, EntityID parent = -1);

    /**
     * @brief Find entity by name
     * @param name Entity name
     * @return EntityID or -1 if not found
     */
    EntityID GetEntityByName(const std::string &name);

    /**
     * @brief Get entity name by ID
     * @param id Entity ID
     * @return Entity name
     */
    const std::string& GetEntityName(EntityID id) const;

    /**
     * @brief Set entity name
     * @param id Entity ID
     * @param name New entity name
     */
    void SetEntityName(EntityID id, const std::string& name);

    /**
     * @brief Get total entity count
     * @return Number of registered entities
     */
    size_t GetEntityCount() const;

    /**
     * @brief Get all entity IDs
     * @return Vector of all entity IDs
     * @note Useful for editor scene tree population
     */
    std::vector<EntityID> GetAllEntities() const;

    /**
     * @brief Destroy entity and remove all components
     * @param id Entity ID to destroy
     */
    void DestroyEntity(EntityID id);

    /**
     * @brief Load scene from YAML file
     * @param src Path to scene YAML file
     * @return true if loaded successfully
     */
    bool LoadScene(const std::string &src);

    /**
     * @brief Register component by entity name
     * @tparam T Component type
     * @param name Entity name
     * @param comp Component data
     */
    template <typename T>
    void RegisterComponent(const std::string &name, T &comp)
    {
        EntityID id = GetEntityByName(name);
        RegisterComponent(id, comp);
    }

    /**
     * @brief Register component by entity ID
     * @tparam T Component type
     * @param id Entity ID
     * @param comp Component data
     */
    template <typename T>
    void RegisterComponent(EntityID id, T &comp)
    {
        auto &componentSet = GetComponentSet<T>();
        componentSet.AddComponent(id, comp);
    }

    /**
     * @brief Check if entity has component
     * @tparam T Component type
     * @param id Entity ID
     * @return true if component exists
     */
    template <typename T>
    bool HasComponent(EntityID id)
    {
        auto &componentSet = GetComponentSet<T>();
        return componentSet.HasComponent(id);
    }

    /**
     * @brief Get component from entity
     * @tparam T Component type
     * @param id Entity ID
     * @return Reference to component
     * @warning No bounds check - ensure HasComponent() first
     */
    template <typename T>
    T &GetComponent(EntityID id)
    {
        SparseSet<T> &components = GetComponentSet<T>();
        T &component = components.GetComponent(id);
        return component;
    }

    /**
     * @brief Get component storage for type
     * @tparam T Component type
     * @return Reference to SparseSet<T>
     */
    template <typename T>
    SparseSet<T> &GetComponentSet();

    std::vector<std::string> entityNames;  ///< Entity name lookup table

    /**
     * @brief Create RenderComponent from shader/mesh paths (Lua helper)
     * @param shaderSrc Path to shader file
     * @param meshSrc Path to mesh file
     * @return Shared pointer to RenderComponent
     */
    static std::shared_ptr<RenderComponent> CreateRenderComponent(const std::string &shaderSrc, const std::string &meshSrc);

    /**
     * @brief Create cube entity (Lua helper)
     * @param cubeComp RenderComponent for cube
     * @param pos Position vector
     * @param color Color vector
     */
    static void CreateCube(std::shared_ptr<RenderComponent> cubeComp, glm::vec3 pos, glm::vec3 color);

    /**
     * @brief Attach Lua script to entity
     * @param entityId Entity ID
     * @param name Script name
     * @param luaClass Lua class table
     */
    static void AttachScript(EntityID entityId, const std::string &name, sol::table luaClass);

#ifdef USE_PYTHON_SCRIPTING
    /**
     * @brief Attach Python script to entity
     * @param entityId Entity ID
     * @param name Script name
     * @param pythonClass Python class object
     */
    static void AttachScript(EntityID entityId, const std::string &name, py::object pythonClass);
#endif

private:
    EntityID i = 0;  ///< Next entity ID counter
    Registry(){};
    Registry(Registry const &) = delete;
    void operator=(Registry const &) = delete;

    SparseSet<HierarchyComponent> HierarchyComponents;  ///< Parent/child relationships
    SparseSet<Lighting> LightingComponents;             ///< Light properties
    SparseSet<RenderComponent> RenderComponents;        ///< Mesh/shader/material
    SparseSet<ScriptComponent> ScriptComponents;        ///< Lua/Python behaviors
    SparseSet<Transform> TransformComponents;           ///< Position/rotation/scale
    SparseSet<Tween> TweenComponents;                   ///< Animation tweens
    SparseSet<WorldTransform> WorldTransformComponents; ///< Computed world matrices
};

#pragma once

#include "components/HierarchyComponent.h"
#include "components/Lighting.h"
#include "components/RenderComponent.h"
#include "components/ScriptComponent.h"
#include "components/Transform.h"
#include "components/Tween.h"

#include <yaml-cpp/yaml.h>

typedef size_t EntityID;

template <typename T>
struct SparseSet
{
public:
    ~SparseSet<T>()
    {
        sparse.clear();
        dense.clear();
        entities.clear();
    };

    void AddComponent(EntityID entity, T &component)
    {
        while (entity >= maxEntities)
        {
            maxEntities *= 2; // Double the maxEntities until it can accommodate the new one
            sparse.resize(maxEntities, -1);
        }

        dense.push_back(component);
        sparse[entity] = dense.size() - 1;
        entities.push_back(entity);
    }

    T &GetComponent(EntityID entity)
    {
        return dense[sparse[entity]];
    }

    void RemoveComponent(EntityID entity)
    {
        // Check if the entity is present
        if (sparse.size() <= entity || sparse[entity] >= dense.size())
            return;

        // Swap-and-pop to maintain dense packing
        size_t indexToRemove = sparse[entity];
        EntityID lastEntity = entities.back();

        // Move last element to removed position
        dense[indexToRemove] = std::move(dense.back());
        entities[indexToRemove] = lastEntity;

        // Update sparse mapping for moved entity
        sparse[lastEntity] = indexToRemove;
        sparse[entity] = static_cast<size_t>(-1);

        // Remove last elements
        dense.pop_back();
        entities.pop_back();
    }

    bool HasComponent(EntityID entity) const
    {
        return entity < sparse.size() &&
               sparse[entity] != static_cast<size_t>(-1) &&
               sparse[entity] < dense.size();
    }

    std::vector<EntityID> GetEntities() { return entities; };

private:
    size_t maxEntities = 100; // dynamically updated as needed
    // the real maximum is size_t's max value

    // sparse contains the indices of the related component
    // e.g. if entity id 6 has a component, sparse[6] will be the index
    // of that component in the dense array
    std::vector<size_t> sparse = std::vector<size_t>(maxEntities, -1);
    std::vector<T> dense;
    std::vector<size_t> entities;
};

class Registry
{
public:
    static Registry &GetInstance()
    {
        static Registry instance;
        return instance;
    }

    void Shutdown();

    // Registers a new entity and corresponding Transform where name = id
    EntityID RegisterEntity(EntityID parent = -1);
    // Registers a new entity and corresponding Transform
    EntityID RegisterEntity(const std::string &name, EntityID parent = -1);
    EntityID GetEntityByName(const std::string &name);
    void DestroyEntity(EntityID id);

    bool LoadScene(const std::string &src);

    template <typename T>
    void RegisterComponent(const std::string &name, T &comp)
    {
        EntityID id = GetEntityByName(name);
        RegisterComponent(id, comp);
    }

    template <typename T>
    void RegisterComponent(EntityID id, T &comp)
    {
        auto &componentSet = GetComponentSet<T>();
        componentSet.AddComponent(id, comp);
    }

    template <typename T>
    bool HasComponent(EntityID id)
    {
        auto &componentSet = GetComponentSet<T>();
        return componentSet.HasComponent(id);
    }

    template <typename T>
    T &GetComponent(EntityID id)
    {
        SparseSet<T> &components = GetComponentSet<T>();
        T &component = components.GetComponent(id);
        return component;
    }

    template <typename T>
    SparseSet<T> &GetComponentSet();

    std::vector<std::string> entityNames;

    // Lua Helper Functions

    static std::shared_ptr<RenderComponent> CreateRenderComponent(const std::string &shaderSrc, const std::string &meshSrc);
    static void CreateCube(std::shared_ptr<RenderComponent> cubeComp, glm::vec3 pos, glm::vec3 color);
    static void AttachScript(EntityID entityId, const std::string &name, sol::table luaClass);
    static void AttachScript(EntityID entityId, const std::string &name, py::object pythonClass);

private:
    EntityID i = 0;
    Registry(){};
    Registry(Registry const &) = delete;
    void operator=(Registry const &) = delete;

    SparseSet<HierarchyComponent> HierarchyComponents;
    SparseSet<Lighting> LightingComponents;
    SparseSet<RenderComponent> RenderComponents;
    SparseSet<ScriptComponent> ScriptComponents;
    SparseSet<Transform> TransformComponents;
    SparseSet<Tween> TweenComponents;
};

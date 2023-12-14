#pragma once

#include "components/Lighting.h"
#include "components/RenderComponent.h"
#include "components/ScriptComponent.h"
#include "components/Transform.h"
#include "components/Tween.h"

#include <yaml-cpp/yaml.h>
#include <sol/sol.hpp>

#define EntityID size_t

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

    void AddComponent(EntityID entity, T component)
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

    // FIXME: this operates at O(N), could be O(1) with more memory usage or something clever
    void RemoveComponent(EntityID entity)
    {
        if (entity >= sparse.size())
        {
            std::cerr << "Attemped to remove component from invalid entity " << entity << std::endl;
            return;
        }

        if (sparse[entity] == std::numeric_limits<size_t>::max())
        {
            std::cerr << "Attempted to remove absent component from entity " << entity << std::endl;
            return;
        }

        size_t temp = sparse[entity];
        sparse[entity] = -1; // implicitly converted to size_t's max value
        dense[temp] = dense.back();
        dense.pop_back();

        // Update the sparse array for the entity that was moved
        for (size_t i = 0; i < sparse.size(); i++)
        {
            if (sparse[i] == dense.size() - 1)
            {
                sparse[i] = temp;
                break;
            }
        }

        entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
    }

    std::vector<EntityID> GetEntities() { return entities; };

private:
    size_t maxEntities = 100; // dynamically updated as needed
    // the real maximum is size_t's max value

    // sparse contains the indices of the related component
    // e.g. if entity id 6 has a component, sparse[6] will be the index
    // of that component in the dense array
    std::vector<size_t> sparse = std::vector<size_t>(maxEntities);
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
    EntityID RegisterEntity();
    // Registers a new entity and corresponding Transform
    EntityID RegisterEntity(const std::string &name);
    EntityID GetEntityByName(const std::string &name);
    void DestroyEntity(EntityID id);

    bool LoadScene(const std::string &src);

    template <typename T>
    void RegisterComponent(const std::string &name, T comp)
    {
        EntityID id = GetEntityByName(name);
        RegisterComponent(id, comp);
    }

    template <typename T>
    void RegisterComponent(EntityID id, T comp)
    {
        auto &componentSet = GetComponentSet<T>();
        componentSet.AddComponent(id, comp);
    }

    template <typename T>
    bool HasComponent(EntityID id)
    {
        auto &componentSet = GetComponentSet<T>();
        return componentSet.find(id) != componentSet.end();
    }

    template <typename T>
    T &GetComponent(EntityID id)
    {
        auto &components = GetComponentSet<T>();
        return components.GetComponent(id);
    }

    template <typename T>
    SparseSet<T> &GetComponentSet();

    std::vector<std::string> entityNames;

    // Lua Helper Functions

    static std::unique_ptr<RenderComponent> CreateRenderComponent(const std::string &shaderSrc, const std::string &meshSrc);
    static void CreateCube(RenderComponent cubeComp, glm::vec3 pos, glm::vec3 color);
    static void AttachScript(EntityID entityId, const std::string &name, sol::table luaClass);

private:
    unsigned int i = 0;
    Registry(){};
    Registry(Registry const &) = delete;
    void operator=(Registry const &) = delete;

    SparseSet<Lighting> LightingComponents;
    SparseSet<RenderComponent> RenderComponents;
    SparseSet<ScriptComponent> ScriptComponents;
    SparseSet<Transform> TransformComponents;
    SparseSet<Tween> TweenComponents;
};

#pragma once

#include "components/CompositeEntity.h"
#include "components/Lighting.h"
#include "components/RenderComponent.h"
#include "components/ScriptComponent.h"
#include "components/Transform.h"
#include "components/Tween.h"

#include <yaml-cpp/yaml.h>
#include <sol/sol.hpp>

#include <memory>
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
    unsigned int RegisterEntity();
    // Registers a new entity and corresponding Transform
    unsigned int RegisterEntity(const std::string &name);
    unsigned int GetEntityByName(const std::string &name);
    void DestroyEntity(unsigned int id);

    bool LoadScene(const std::string &src);
    static std::shared_ptr<RenderComponent> CreateRenderComponent(const std::string &shaderSrc, const std::string &meshSrc);
    static void CreateCube(std::shared_ptr<RenderComponent> cubeComp, glm::vec3 pos, glm::vec3 color);
    static void AttachScript(unsigned int entityId, const std::string &name, sol::table luaClass);

    template <typename T>
    void RegisterComponent(const std::string &name, std::shared_ptr<T> comp)
    {
        unsigned int id = GetEntityByName(name);
        RegisterComponent(id, comp);
    }

    template <typename T>
    void RegisterComponent(unsigned int id, std::shared_ptr<T> comp)
    {
        auto &componentMap = GetComponentMap<T>();
        componentMap[id] = comp;
    }

    template <typename T>
    std::shared_ptr<T> GetComponent(unsigned int entityID)
    {
        auto &componentMap = GetComponentMap<T>();
        auto it = componentMap.find(entityID);
        return (it != componentMap.end()) ? it->second : nullptr;
    }

    template <typename T>
    std::unordered_map<unsigned int, std::shared_ptr<T>> &GetComponentMap();

    std::unordered_map<unsigned int, std::string> entities;

private:
    unsigned int i = 0;
    Registry(){};
    Registry(Registry const &) = delete;
    void operator=(Registry const &) = delete;

    // TODO: consider refactor to unique or raw ptrs
    std::unordered_map<unsigned int, std::shared_ptr<CompositeEntity>> CompositeComponents;
    std::unordered_map<unsigned int, std::shared_ptr<Lighting>> LightingComponents;
    std::unordered_map<unsigned int, std::shared_ptr<RenderComponent>> RenderComponents;
    std::unordered_map<unsigned int, std::shared_ptr<ScriptComponent>> ScriptComponents;
    std::unordered_map<unsigned int, std::shared_ptr<Transform>> TransformComponents;
    std::unordered_map<unsigned int, std::shared_ptr<Tween>> TweenComponents;
};

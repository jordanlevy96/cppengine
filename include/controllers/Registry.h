#pragma once

#include "components/CompositeEntity.h"
#include "components/Lighting.h"
#include "components/RenderComponent.h"
#include "components/Transform.h"
#include "util/globals.h"

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

    unsigned int RegisterEntity();
    unsigned int RegisterEntity(const std::string &name);
    unsigned int GetEntityByName(const std::string &name);
    void DestroyEntity(unsigned int id);

    bool LoadScene(const std::string &src);

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

    std::unordered_map<unsigned int, std::shared_ptr<CompositeEntity>> CompositeComponents;
    std::unordered_map<unsigned int, std::shared_ptr<Lighting>> LightingComponents;
    std::unordered_map<unsigned int, std::shared_ptr<RenderComponent>> RenderComponents;
    std::unordered_map<unsigned int, std::shared_ptr<Transform>> TransformComponents;
};
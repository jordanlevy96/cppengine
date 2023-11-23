#pragma once

#include "components/Emitter.h"
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

    Registry(Registry const &) = delete;
    void operator=(Registry const &) = delete;

    unsigned int RegisterEntity(const std::string &name);
    void DestroyEntity(unsigned int id);
    unsigned int GetEntityByName(const std::string &name);
    void RegisterComponent(unsigned int id, std::shared_ptr<Component> comp, ComponentTypes type);
    void RegisterComponent(const std::string &name, std::shared_ptr<Component> comp, ComponentTypes type);
    bool LoadScene(const std::string &src);

    std::unordered_map<unsigned int, std::shared_ptr<Lighting>> LightingComponents;
    std::unordered_map<unsigned int, std::shared_ptr<RenderComponent>> RenderComponents;
    std::unordered_map<unsigned int, std::shared_ptr<Transform>> TransformComponents;
    std::unordered_map<unsigned int, std::shared_ptr<Emitter>> EmitterComponents;
    std::unordered_map<unsigned int, std::string> entities;

private:
    unsigned int i = 0;
    Registry(){};
};
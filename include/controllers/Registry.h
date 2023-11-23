#pragma once

#include "components/Emitter.h"
#include "components/Lighting.h"
#include "components/RenderComponent.h"
#include "components/Transform.h"

#include <yaml-cpp/yaml.h>
#include <sol/sol.hpp>

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
    unsigned int GetEntityByName(const std::string &name);
    void RegisterComponent(unsigned int id, Component *comp, ComponentTypes type);
    void RegisterComponent(const std::string &name, Component *comp, ComponentTypes type);

    std::unordered_map<unsigned int, Lighting *> LightingComponents;
    std::unordered_map<unsigned int, RenderComponent *> RenderComponents;
    std::unordered_map<unsigned int, Transform *> TransformComponents;
    std::unordered_map<unsigned int, Emitter *> EmitterComponents;
    std::vector<unsigned int> entities;
    std::unordered_map<unsigned int, std::string> entityNames;

private:
    unsigned int i = 0;
    Registry(){};
};
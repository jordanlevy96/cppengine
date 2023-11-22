#pragma once

#include "components/Component.h"

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

    unsigned int RegisterEntity(const std::string &name)
    {
        entities[i] = name;
        return i++;
    }

    unsigned int GetEntityByName(const std::string &name)
    {
        unsigned int index;
        auto it = std::find(entities.begin(), entities.end(), name);
        if (it == entities.end())
        {
            // name not in vector
            index = -1;
        }
        else
        {
            index = std::distance(entities.begin(), it);
        }

        return index;
    }

    void RegisterComponent(unsigned int id, Component *comp)
    {
        components[comp->GetName()][id] = comp;
    }

    void RegisterComponent(const std::string &name, Component *comp)
    {
        unsigned int id = GetEntityByName(name);
        RegisterComponent(id, comp);
    }

    // void LoadScene(const std::string &src)
    // {
    //     try
    //     {
    //         YAML::Node yaml = YAML::LoadFile("example.yaml");

    //         const YAML::Node &objectsNode = yaml["scene"]["objects"];
    //         for (const auto &objectNode : objectsNode)
    //         {
    //             const std::string script, model;
    //             script = objectNode["shader"].as<std::string>();
    //             model = objectNode["model"].as<std::string>();

    //             Transform transform;
    //             if (objectNode["transform"]["pos"])
    //             {
    //                 transform.pos.x = objectNode["transform"]["pos"]["x"].as<float>();
    //                 transform.pos.y = objectNode["transform"]["pos"]["y"].as<float>();
    //                 transform.pos.z = objectNode["transform"]["pos"]["z"].as<float>();
    //             }
    //             if (objectNode["transform"]["scale"])
    //             {
    //                 transform.scale.x = objectNode["transform"]["scale"]["x"].as<float>();
    //                 transform.scale.y = objectNode["transform"]["scale"]["y"].as<float>();
    //                 transform.scale.z = objectNode["transform"]["scale"]["z"].as<float>();
    //             }

    //             obj.color.r = objectNode["color"]["r"].as<float>();
    //             obj.color.g = objectNode["color"]["g"].as<float>();
    //             obj.color.b = objectNode["color"]["b"].as<float>();

    //             const YAML::Node &componentsNode = objectNode["components"];
    //             for (const auto &componentNode : componentsNode)
    //             {
    //                 obj.components.push_back(componentNode.as<std::string>());
    //             }

    //             scene.objects.push_back(obj);
    //         }

    //         // Process the parsed data (you can do whatever you need with the C++ structures)
    //         // ...
    //     }
    //     catch (const YAML::Exception &e)
    //     {
    //         std::cerr << "YAML parsing error: " << e.what() << std::endl;
    //         return 1;
    //     }
    // }

private:
    unsigned int i = 0;
    std::vector<std::string> entities;
    std::unordered_map<std::string, std::unordered_map<unsigned int, Component *>> components;
    Registry(){};
};
#include "controllers/Registry.h"

unsigned int Registry::RegisterEntity(const std::string &name)
{
    entityNames[i] = name;
    entities.push_back(i);
    return i++;
}

unsigned int Registry::GetEntityByName(const std::string &name)
{
    for (const auto &pair : entityNames)
    {
        if (pair.second == name)
        {
            return pair.first;
        }
    }

    return -1;
}

void Registry::RegisterComponent(unsigned int id, Component *comp, ComponentTypes type)
{
    switch (type)
    {
    case (ComponentTypes::TransformType):
        TransformComponents[id] = static_cast<Transform *>(comp);
        break;
    case ComponentTypes::RenderComponentType:
        RenderComponents[id] = static_cast<RenderComponent *>(comp);
        break;
    case ComponentTypes::LightingType:
        LightingComponents[id] = static_cast<Lighting *>(comp);
        break;
    case ComponentTypes::EmitterType:
        EmitterComponents[id] = static_cast<Emitter *>(comp);
        break;
    default:
        std::cerr << "Component not set up for registry!" << std::endl;
        break;
    }
}

void Registry::RegisterComponent(const std::string &name, Component *comp, ComponentTypes type)
{
    unsigned int id = GetEntityByName(name);
    RegisterComponent(id, comp, type);
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
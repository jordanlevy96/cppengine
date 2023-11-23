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

bool Registry::LoadScene(const std::string &src)
{
    try
    {
        YAML::Node yaml = YAML::LoadFile(src);

        const YAML::Node &objectsNode = yaml["scene"]["objects"];
        for (const auto &objectNode : objectsNode)
        {
            const std::string &name = objectNode["name"].as<std::string>();
            unsigned int id = RegisterEntity(name);
            Transform *transform = new Transform();
            RegisterComponent(id, transform, ComponentTypes::TransformType);
            if (objectNode["transform"]["pos"])
            {
                transform->Pos.x = objectNode["transform"]["pos"]["x"].as<float>();
                transform->Pos.y = objectNode["transform"]["pos"]["y"].as<float>();
                transform->Pos.z = objectNode["transform"]["pos"]["z"].as<float>();
            }
            if (objectNode["transform"]["scale"])
            {
                transform->Scale.x = objectNode["transform"]["scale"]["x"].as<float>();
                transform->Scale.y = objectNode["transform"]["scale"]["y"].as<float>();
                transform->Scale.z = objectNode["transform"]["scale"]["z"].as<float>();
            }
            if (objectNode["transform"]["color"])
            {
                transform->Color.r = objectNode["transform"]["color"]["r"].as<float>();
                transform->Color.g = objectNode["transform"]["color"]["g"].as<float>();
                transform->Color.b = objectNode["transform"]["color"]["b"].as<float>();
            }

            if (objectNode["components"])
            {
                const YAML::Node &componentsNode = objectNode["components"];
                for (const auto &componentNode : componentsNode)
                {
                    const std::string name = componentNode["name"].as<std::string>();
                    if (name == "lighting")
                    {
                        const std::string &shaderSrc = (const std::string &)(RES_PATH) + "/shaders/" + componentNode["shader"].as<std::string>();
                        const std::string &modelSrc = (const std::string &)(RES_PATH) + "/models/" + componentNode["model"].as<std::string>();
                        const std::string &lightName = componentNode["light"].as<std::string>();

                        unsigned int light = GetEntityByName(lightName);
                        Transform *lightTrans = TransformComponents[light];
                        Component *lightComp = new Lighting(shaderSrc, modelSrc, &transform->Color, lightTrans);
                        RegisterComponent(id, lightComp, ComponentTypes::LightingType);
                    }
                    else if (name == "emitter")
                    {
                        const std::string &shaderSrc = (const std::string &)(RES_PATH) + "/shaders/" + componentNode["shader"].as<std::string>();
                        const std::string &modelSrc = (const std::string &)(RES_PATH) + "/models/" + componentNode["model"].as<std::string>();
                        Component *emitter = new Emitter(shaderSrc, modelSrc);
                        RegisterComponent(id, emitter, ComponentTypes::EmitterType);
                    }
                }
            }
        }
        return true;
    }
    catch (const YAML::Exception &e)
    {
        std::cerr << "YAML parsing error: " << e.what() << std::endl;
        return false;
    }
}
#include "controllers/Registry.h"

unsigned int Registry::RegisterEntity(const std::string &name)
{
    entities[i] = name;
    return i++;
}

void Registry::DestroyEntity(unsigned int id)
{
    LightingComponents.erase(id);
    RenderComponents.erase(id);
    TransformComponents.erase(id);
}

unsigned int Registry::GetEntityByName(const std::string &name)
{
    for (const auto &pair : entities)
    {
        if (pair.second == name)
        {
            return pair.first;
        }
    }

    return -1;
}

void Registry::RegisterComponent(unsigned int id, std::shared_ptr<Component> comp, ComponentTypes type)
{
    switch (type)
    {
    case (ComponentTypes::TransformType):
        TransformComponents[id] = std::dynamic_pointer_cast<Transform>(comp);
        break;
    case ComponentTypes::RenderComponentType:
        RenderComponents[id] = std::dynamic_pointer_cast<RenderComponent>(comp);
        break;
    case ComponentTypes::LightingType:
        LightingComponents[id] = std::dynamic_pointer_cast<Lighting>(comp);
        break;
    case ComponentTypes::EmitterType:
        EmitterComponents[id] = std::dynamic_pointer_cast<Emitter>(comp);
        break;
    default:
        std::cerr << "Component not set up for registry!" << std::endl;
        break;
    }
}

void Registry::RegisterComponent(const std::string &name, std::shared_ptr<Component> comp, ComponentTypes type)
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
            std::shared_ptr<Transform> transform = std::make_shared<Transform>();
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
                        std::shared_ptr<Transform> lightTrans = TransformComponents[light];
                        std::shared_ptr<Component> lightComp = std::make_shared<Lighting>(shaderSrc, modelSrc, &transform->Color, lightTrans);
                        RegisterComponent(id, lightComp, ComponentTypes::LightingType);
                    }
                    else if (name == "emitter")
                    {
                        const std::string &shaderSrc = (const std::string &)(RES_PATH) + "/shaders/" + componentNode["shader"].as<std::string>();
                        const std::string &modelSrc = (const std::string &)(RES_PATH) + "/models/" + componentNode["model"].as<std::string>();
                        std::shared_ptr<Component> emitter = std::make_shared<Emitter>(shaderSrc, modelSrc);
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
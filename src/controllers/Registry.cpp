#include "controllers/Registry.h"
#include "controllers/ScriptManager.h"

unsigned int Registry::RegisterEntity()
{
    std::string name = std::to_string(i);

    return RegisterEntity(name);
}

unsigned int Registry::RegisterEntity(const std::string &name)
{
    entities[i] = name;
    std::shared_ptr<Transform> t = std::make_shared<Transform>();
    RegisterComponent(i, t);
    return i++;
}

void Registry::DestroyEntity(unsigned int id)
{
    CompositeComponents.erase(id);
    LightingComponents.erase(id);
    RenderComponents.erase(id);
    TransformComponents.erase(id);
    entities.erase(id);
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

template <>
std::unordered_map<unsigned int, std::shared_ptr<CompositeEntity>> &Registry::GetComponentMap<CompositeEntity>()
{
    return CompositeComponents;
}

template <>
std::unordered_map<unsigned int, std::shared_ptr<Lighting>> &Registry::GetComponentMap<Lighting>()
{
    return LightingComponents;
}

template <>
std::unordered_map<unsigned int, std::shared_ptr<RenderComponent>> &Registry::GetComponentMap<RenderComponent>()
{
    return RenderComponents;
}

template <>
std::unordered_map<unsigned int, std::shared_ptr<ScriptComponent>> &Registry::GetComponentMap<ScriptComponent>()
{
    return ScriptComponents;
}

template <>
std::unordered_map<unsigned int, std::shared_ptr<Transform>> &Registry::GetComponentMap<Transform>()
{
    return TransformComponents;
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
            std::shared_ptr<Transform> transform = GetComponent<Transform>(id);
            if (objectNode["transform"])
            {
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
            }

            if (objectNode["components"])
            {
                const YAML::Node &componentsNode = objectNode["components"];
                for (const auto &componentNode : componentsNode)
                {
                    const std::string componentType = componentNode["type"].as<std::string>();
                    if (componentType == "Lighting")
                    {
                        const std::string &shaderSrc = (const std::string &)(RES_PATH) + "/shaders/" + componentNode["shader"].as<std::string>();
                        const std::string &modelSrc = (const std::string &)(RES_PATH) + "/models/" + componentNode["model"].as<std::string>();
                        const std::string &lightName = componentNode["light"].as<std::string>();

                        unsigned int light = GetEntityByName(lightName);
                        std::shared_ptr<Transform> lightTrans = GetComponent<Transform>(light);
                        std::shared_ptr<RenderComponent> rc = std::make_shared<RenderComponent>(shaderSrc, modelSrc);
                        std::shared_ptr<Lighting> lightComp = std::make_shared<Lighting>(rc, &transform->Color, lightTrans);
                        RegisterComponent<Lighting>(id, lightComp);
                    }
                    else if (componentType == "RenderComponent")
                    {
                        const std::string &shaderSrc = (const std::string &)(RES_PATH) + "/shaders/" + componentNode["shader"].as<std::string>();
                        const std::string &modelSrc = (const std::string &)(RES_PATH) + "/models/" + componentNode["model"].as<std::string>();
                        std::shared_ptr<RenderComponent> rc = std::make_shared<RenderComponent>(shaderSrc, modelSrc);
                        RegisterComponent<RenderComponent>(id, rc);
                    }
                    else if (componentType == "Script")
                    {
                        const std::string &scriptSrc = (const std::string &)(RES_PATH) + "/scripts/" + componentNode["script"].as<std::string>();
                        ScriptManager &sm = ScriptManager::GetInstance();
                        // Load the script and call the ready function
                        // Be careful with this ready call, if it relies on stuff that hasn't been initialized yet, it'll fail.
                        sm.Run(scriptSrc);
                        sol::table scriptClass = sm.GetLuaTable(name);
                        scriptClass["ready"]();
                        std::shared_ptr<ScriptComponent> sc = std::make_shared<ScriptComponent>(scriptSrc, scriptClass);
                        RegisterComponent<ScriptComponent>(id, sc);
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

#include "controllers/App.h"
#include "controllers/Registry.h"
#include "controllers/ScriptManager.h"

void Registry::Shutdown()
{
    for (size_t i; i < entityNames.size(); i++)
    {
        DestroyEntity(i);
    }
}

EntityID Registry::RegisterEntity()
{
    std::string name = std::to_string(i);

    return RegisterEntity(name);
}

EntityID Registry::RegisterEntity(const std::string &name)
{
    entityNames.push_back(name);
    Transform t = Transform();
    RegisterComponent(i, t);
    return i++;
}

void Registry::DestroyEntity(EntityID id)
{
    LightingComponents.RemoveComponent(id);
    RenderComponents.RemoveComponent(id);
    ScriptComponents.RemoveComponent(id);
    TransformComponents.RemoveComponent(id);
    TweenComponents.RemoveComponent(id);
}

EntityID Registry::GetEntityByName(const std::string &name)
{
    auto it = std::find(entityNames.begin(), entityNames.end(), name);
    if (it == entityNames.end())
    {
        return -1;
    }
    else
    {
        return std::distance(entityNames.begin(), it);
    }
}

template <>
SparseSet<Lighting> &Registry::GetComponentSet<Lighting>()
{
    return LightingComponents;
}

template <>
SparseSet<RenderComponent> &Registry::GetComponentSet<RenderComponent>()
{
    return RenderComponents;
}

template <>
SparseSet<ScriptComponent> &Registry::GetComponentSet<ScriptComponent>()
{
    return ScriptComponents;
}

template <>
SparseSet<Transform> &Registry::GetComponentSet<Transform>()
{
    return TransformComponents;
}

template <>
SparseSet<Tween> &Registry::GetComponentSet<Tween>()
{
    return TweenComponents;
}

bool Registry::LoadScene(const std::string &src)
{
    const std::string &res = App::GetInstance().conf.ResourcePath;
    try
    {
        YAML::Node yaml = YAML::LoadFile(res + src);

        const YAML::Node &objectsNode = yaml["scene"]["objects"];
        for (const auto &objectNode : objectsNode)
        {
            const std::string &name = objectNode["name"].as<std::string>();
            EntityID id = RegisterEntity(name);
            Transform transform = GetComponent<Transform>(id);
            if (objectNode["transform"])
            {
                if (objectNode["transform"]["pos"])
                {
                    transform.Pos.x = objectNode["transform"]["pos"]["x"].as<float>();
                    transform.Pos.y = objectNode["transform"]["pos"]["y"].as<float>();
                    transform.Pos.z = objectNode["transform"]["pos"]["z"].as<float>();
                }
                if (objectNode["transform"]["scale"])
                {
                    transform.Scale.x = objectNode["transform"]["scale"]["x"].as<float>();
                    transform.Scale.y = objectNode["transform"]["scale"]["y"].as<float>();
                    transform.Scale.z = objectNode["transform"]["scale"]["z"].as<float>();
                }
                if (objectNode["transform"]["color"])
                {
                    transform.Color.r = objectNode["transform"]["color"]["r"].as<float>();
                    transform.Color.g = objectNode["transform"]["color"]["g"].as<float>();
                    transform.Color.b = objectNode["transform"]["color"]["b"].as<float>();
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
                        const std::string &shaderSrc = (const std::string &)(res) + "/shaders/" + componentNode["shader"].as<std::string>();
                        const std::string &modelSrc = (const std::string &)(res) + "/models/" + componentNode["model"].as<std::string>();
                        const std::string &lightName = componentNode["light"].as<std::string>();

                        EntityID light = GetEntityByName(lightName);
                        RenderComponent rc = RenderComponent(shaderSrc, modelSrc);
                        RegisterComponent<RenderComponent>(id, rc);
                        Lighting lightComp = Lighting(&GetComponent<Transform>(light));
                        RegisterComponent<Lighting>(id, lightComp);
                    }
                    else if (componentType == "RenderComponent")
                    {
                        const std::string &shaderSrc = (const std::string &)(res) + "/shaders/" + componentNode["shader"].as<std::string>();
                        const std::string &modelSrc = (const std::string &)(res) + "/models/" + componentNode["model"].as<std::string>();
                        RenderComponent rc = RenderComponent(shaderSrc, modelSrc);
                        RegisterComponent<RenderComponent>(id, rc);
                    }
                    else if (componentType == "Script")
                    {
                        const std::string &scriptSrc = (const std::string &)(res) + "/scripts/" + componentNode["script"].as<std::string>();
                        ScriptManager &sm = ScriptManager::GetInstance();
                        // Load the script and call the ready function
                        // Be careful with this ready call, if it relies on stuff that hasn't been initialized yet, it'll fail.
                        sm.Run(scriptSrc);
                        sol::table scriptClass = sm.GetLuaTable(name);
                        for (const auto &property : componentNode)
                        {
                            std::string key = property.first.as<std::string>();
                            // "type" and "script" are already handled above
                            if (key == "type" || key == "script")
                            {
                                continue;
                            }

                            // Assume all other keys are meant as strings for the Lua script
                            scriptClass[key] = property.second.as<std::string>();
                        }

                        ScriptComponent sc = ScriptComponent(scriptSrc, scriptClass);
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

// For Lua scripting

void Registry::AttachScript(EntityID entityId, const std::string &name, sol::table luaClass)
{
    ScriptComponent sc = ScriptComponent(name, luaClass);

    Registry *r = &GetInstance();
    r->RegisterComponent<ScriptComponent>(entityId, sc);
}

std::unique_ptr<RenderComponent> Registry::CreateRenderComponent(const std::string &shaderSrc, const std::string &meshSrc)
{
    const std::string &res = App::GetInstance().conf.ResourcePath;
    std::string shaderPath = (res) + "shaders/" + shaderSrc;
    std::string meshPath = (res) + "models/" + meshSrc;

    return std::make_unique<RenderComponent>(shaderPath, meshPath);
}

void Registry::CreateCube(RenderComponent cubeComp, glm::vec3 pos, glm::vec3 color)
{
    // static reference to registry for Lua binding
    Registry *r = &GetInstance();
    EntityID id = r->RegisterEntity();
    Transform transform = r->GetComponent<Transform>(id);
    transform.Pos = pos;
    transform.Color = color;
    r->RegisterComponent<RenderComponent>(id, cubeComp);
}
/**
 * @file SceneContext.cpp
 * @brief Implementation of SceneContext dependency injection
 */

#include "systems/SceneContext.h"
#include "controllers/Game.h"
#include "controllers/Registry.h"
#include "controllers/WindowManager.h"
#include "systems/HTMLRendererMT.h"
#include "systems/ReactiveUI.h"
#include "util/Logger.h"

SceneContext::SceneContext(const std::string &sceneName,
                           const std::unordered_set<std::string> &capabilities)
    : m_sceneName(sceneName), m_capabilities(capabilities)
{
}

bool SceneContext::HasCapability(const std::string &capability) const
{
    return m_capabilities.find(capability) != m_capabilities.end();
}

std::vector<std::string> SceneContext::GetMissingCapabilities(
    const std::vector<std::string> &required) const
{
    std::vector<std::string> missing;
    for (const auto &cap : required)
    {
        if (!HasCapability(cap))
        {
            missing.push_back(cap);
        }
    }
    return missing;
}

void SceneContext::RegisterRole(const std::string &role, sol::object value)
{
    if (m_roleValues.find(role) != m_roleValues.end())
    {
        LOG_WARNING("[SceneContext] Role '{}' already registered - overwriting", role);
    }
    m_roleValues[role] = value;
}

void SceneContext::UpdateContextWithRole(sol::state &lua, sol::table &ctxTable,
                                         const std::string &role, sol::object value)
{
    ctxTable[role] = value;
    RegisterRole(role, value);
}

sol::table SceneContext::ToLuaTable(sol::state &lua)
{
    LOG_DEBUG("[SceneContext] ToLuaTable: creating table...");
    sol::table ctx = lua.create_table();

    LOG_DEBUG("[SceneContext] ToLuaTable: building engine table...");
    // Always present: ctx.engine
    ctx["engine"] = BuildEngineTable(lua);

    LOG_DEBUG("[SceneContext] ToLuaTable: building scene table...");
    // Always present: ctx.scene
    ctx["scene"] = BuildSceneTable(lua);

    LOG_DEBUG("[SceneContext] ToLuaTable: building capability tables...");
    // Optional capabilities - only present if declared
    static const std::vector<std::string> knownCapabilities = {
        "ui", "input", "audio", "world"};

    for (const auto &cap : knownCapabilities)
    {
        LOG_DEBUG("[SceneContext] ToLuaTable: checking capability '{}'...", cap);
        sol::object capTable = BuildCapabilityTable(lua, cap);
        LOG_DEBUG("[SceneContext] ToLuaTable: got result for '{}', checking type...", cap);
        if (capTable.valid() && capTable.get_type() != sol::type::nil)
        {
            LOG_DEBUG("[SceneContext] ToLuaTable: adding '{}' to context", cap);
            ctx[cap] = capTable;
        }
        LOG_DEBUG("[SceneContext] ToLuaTable: done with '{}'", cap);
        // If not enabled, ctx.{cap} will be nil (not present)
    }

    LOG_DEBUG("[SceneContext] ToLuaTable: adding role values...");
    // Role-provided values (registered modules)
    for (const auto &[role, value] : m_roleValues)
    {
        ctx[role] = value;
    }

    LOG_DEBUG("[SceneContext] ToLuaTable: done");
    return ctx;
}

sol::table SceneContext::BuildEngineTable(sol::state &lua)
{
    sol::table engine = lua.create_table();

    // Game timing - return lambda that fetches current delta
    engine["getDelta"] = []() -> double
    {
        return Game::GetInstance().delta;
    };

    // Resource path accessor
    engine["getResourcePath"] = []() -> std::string
    {
        return Game::GetInstance().conf.ResourcePath;
    };

    // Camera access (returns pointer, may be nullptr)
    engine["getCamera"] = []() -> Camera *
    {
        return Game::GetInstance().cam;
    };

    // Registry access for entity operations
    engine["getRegistry"] = []() -> Registry &
    {
        return Registry::GetInstance();
    };

    // Logging utilities
    engine["log"] = [](const std::string &msg)
    {
        LOG_INFO("[Script] {}", msg);
    };

    engine["logWarning"] = [](const std::string &msg)
    {
        LOG_WARNING("[Script] {}", msg);
    };

    engine["logError"] = [](const std::string &msg)
    {
        LOG_ERROR("[Script] {}", msg);
    };

    return engine;
}

sol::table SceneContext::BuildSceneTable(sol::state &lua)
{
    sol::table scene = lua.create_table();

    scene["name"] = m_sceneName;

    // Capture this for lambda
    scene["hasCapability"] = [this](const std::string &cap) -> bool
    {
        return HasCapability(cap);
    };

    scene["isStrictMode"] = [this]() -> bool
    {
        return IsStrictMode();
    };

    return scene;
}

sol::object SceneContext::BuildCapabilityTable(sol::state &lua,
                                               const std::string &capability)
{
    LOG_DEBUG("[SceneContext] BuildCapabilityTable: checking if '{}' is enabled...", capability);
    bool hasIt = HasCapability(capability);
    LOG_DEBUG("[SceneContext] BuildCapabilityTable: hasCapability('{}') = {}", capability, hasIt);

    if (!hasIt)
    {
        LOG_DEBUG("[SceneContext] BuildCapabilityTable: '{}' not enabled, returning nil", capability);
        return sol::nil;
    }

    LOG_DEBUG("[SceneContext] BuildCapabilityTable: building table for '{}'...", capability);
    sol::table cap = lua.create_table();

    if (capability == "ui")
    {
        // UI capability - access to reactive UI state
        // Mirrors the existing SetUIValue/RefreshUI bindings
        cap["setValue"] = [](const std::string &key, sol::object value)
        {
            ReactiveUI &reactiveUI = ReactiveUI::GetInstance();
            auto luaState = reactiveUI.GetLuaState();
            if (luaState)
            {
                if (value.is<int>())
                {
                    luaState->SetValue(key, value.as<int>());
                }
                else if (value.is<double>())
                {
                    luaState->SetValue(key, value.as<double>());
                }
                else if (value.is<std::string>())
                {
                    luaState->SetValue(key, value.as<std::string>());
                }
                else if (value.is<bool>())
                {
                    luaState->SetValue(key, value.as<bool>());
                }
            }
        };

        cap["refresh"] = []()
        {
            ReactiveUI &reactiveUI = ReactiveUI::GetInstance();
            HTMLRendererMT &htmlRenderer = HTMLRendererMT::GetInstance();
            htmlRenderer.UpdateHTML(reactiveUI.GetRenderedHTML());
        };

        return cap;
    }
    else if (capability == "input")
    {
        // Input capability - access to input state
        cap["isKeyPressed"] = [](const std::string &key) -> bool
        {
            return WindowManager::GetInstance().IsKeyPressed(key);
        };

        return cap;
    }
    else if (capability == "audio")
    {
        // Audio capability - placeholder for future audio system
        LOG_DEBUG("[SceneContext] Audio capability declared but not yet implemented");
        return sol::nil;
    }
    else if (capability == "world")
    {
        // World capability - placeholder for world simulation service
        LOG_DEBUG("[SceneContext] World capability declared but not yet implemented");
        return sol::nil;
    }

    // Unknown capability - log and return nil
    LOG_WARNING("[SceneContext] Unknown capability '{}' requested", capability);
    return sol::nil;
}

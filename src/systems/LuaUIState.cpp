#include "systems/LuaUIState.h"
#include "util/Logger.h"
#include "controllers/ScriptManager.h"
#include "util/Logger.h"
#include <sstream>

LuaUIState::LuaUIState()
    : m_lua(nullptr), m_isDirty(true), m_isReady(false)
{
    // Get reference to ScriptManager's Lua VM
    ScriptManager& scriptMgr = ScriptManager::GetInstance();
    m_lua = &scriptMgr.GetLuaState();
}

bool LuaUIState::LoadStateFile(const std::string& path) {
    if (!m_lua) {
        LOG_ERROR("[LuaUIState] Lua state not initialized");
        return false;
    }

    try {
        LOG_INFO("[LuaUIState] Loading state file: {}", path);

        // Execute the Lua file which should return a table
        sol::load_result loadResult = m_lua->load_file(path);

        if (!loadResult.valid()) {
            sol::error err = loadResult;
            LOG_ERROR("[LuaUIState] Failed to load file: {}", err.what());
            return false;
        }

        // Call the loaded chunk to get the returned table
        sol::protected_function_result result = loadResult();

        if (!result.valid()) {
            sol::error err = result;
            LOG_ERROR("[LuaUIState] Failed to execute file: {}", err.what());
            return false;
        }

        // The result should be a table
        if (!result[0].is<sol::table>()) {
            LOG_ERROR("[LuaUIState] File did not return a table");
            return false;
        }

        m_stateTable = result[0];
        m_isReady = true;
        m_isDirty = true;

        LOG_INFO("[LuaUIState] State file loaded successfully");
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("[LuaUIState] Exception loading state file: {}", e.what());
        return false;
    }
}

sol::object LuaUIState::GetValue(const std::string& key) {
    if (!m_isReady) {
        return sol::nil;
    }

    return NavigatePath(key);
}

bool LuaUIState::EvaluateCondition(const std::string& expression) {
    if (!m_isReady || !m_lua) {
        return false;
    }

    try {
        // Create a Lua function that evaluates the expression
        // Set the state table as the environment so expressions can access data directly
        std::string luaCode = "return function() return " + expression + " end";

        sol::load_result loadResult = m_lua->load(luaCode);
        if (!loadResult.valid()) {
            sol::error err = loadResult;
            LOG_ERROR("[LuaUIState] Failed to load expression: {}", err.what());
            return false;
        }

        sol::protected_function func = loadResult();

        // Set the state table as the environment for the function
        sol::environment env(*m_lua, sol::create, m_stateTable);
        sol::set_environment(env, func);

        sol::protected_function_result result = func();

        if (!result.valid()) {
            sol::error err = result;
            LOG_ERROR("[LuaUIState] Failed to evaluate expression: {}", err.what());
            return false;
        }

        // Convert result to boolean
        sol::object obj = result[0];

        if (obj.is<bool>()) {
            return obj.as<bool>();
        } else if (obj.is<int>() || obj.is<double>()) {
            // Lua truthiness: numbers are true if non-zero
            double value = obj.as<double>();
            return value != 0.0;
        } else if (obj.is<std::string>()) {
            // Non-empty strings are true
            return !obj.as<std::string>().empty();
        } else if (!obj.valid() || obj.get_type() == sol::type::lua_nil) {
            return false;
        }

        // Default: truthy if not nil
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("[LuaUIState] Exception evaluating condition: {}", e.what());
        return false;
    }
}

std::string LuaUIState::EvaluateAsString(const std::string& expression) {
    if (!m_isReady || !m_lua) {
        LOG_WARNING("[LuaUIState] Cannot evaluate '{}': state not ready", expression);
        return "";
    }

    static bool loggedViewportImage = false;
    bool isViewportImage = (expression == "viewportImage");

    try {
        // Create a Lua function that evaluates the expression
        // Set the state table as the environment so expressions can access data directly
        std::string luaCode = "return function() return " + expression + " end";

        sol::load_result loadResult = m_lua->load(luaCode);
        if (!loadResult.valid()) {
            sol::error err = loadResult;
            LOG_ERROR("[LuaUIState] Failed to load expression '{}': {}", expression, err.what());
            return "";
        }

        sol::protected_function func = loadResult();

        // Set the state table as the environment for the function
        sol::environment env(*m_lua, sol::create, m_stateTable);
        sol::set_environment(env, func);

        sol::protected_function_result result = func();

        if (!result.valid()) {
            sol::error err = result;
            LOG_ERROR("[LuaUIState] Failed to evaluate expression '{}': {}", expression, err.what());
            return "";
        }

        // Convert result to string
        sol::object obj = result[0];

        if (obj.is<std::string>()) {
            std::string value = obj.as<std::string>();
            if (isViewportImage && !loggedViewportImage) {
                LOG_INFO("[LuaUIState] ✓ Successfully evaluated 'viewportImage': {} bytes", value.size());
                loggedViewportImage = true;
            }
            return value;
        } else if (obj.is<int>()) {
            return std::to_string(obj.as<int>());
        } else if (obj.is<double>()) {
            return std::to_string(obj.as<double>());
        } else if (obj.is<bool>()) {
            return obj.as<bool>() ? "true" : "false";
        } else if (!obj.valid() || obj.get_type() == sol::type::lua_nil) {
            if (isViewportImage) {
                LOG_ERROR("[LuaUIState] 'viewportImage' evaluates to nil!");
            }
            return "";
        }

        // For other types, try to convert to string
        return "<object>";
    }
    catch (const std::exception& e) {
        LOG_ERROR("[LuaUIState] Exception evaluating '{}' as string: {}", expression, e.what());
        return "";
    }
}

sol::object LuaUIState::NavigatePath(const std::string& path) {
    if (!m_isReady) {
        return sol::nil;
    }

    // Split path by dots and navigate through tables
    std::istringstream iss(path);
    std::string key;
    sol::object current = m_stateTable;

    while (std::getline(iss, key, '.')) {
        if (!current.is<sol::table>()) {
            // Can't navigate further, not a table
            return sol::nil;
        }

        sol::table table = current.as<sol::table>();
        current = table[key];

        if (!current.valid() || current.get_type() == sol::type::lua_nil) {
            // Key doesn't exist
            LOG_ERROR("[LuaUIState] Key not found: {} in path: {}", key, path);
            return sol::nil;
        }
    }

    return current;
}

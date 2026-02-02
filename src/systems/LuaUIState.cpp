/**
 * @file LuaUIState.cpp
 * @brief Reactive UI state manager backed by Lua VM
 * @lines ~275
 *
 * Purpose: Manages UI data loaded from Lua files with automatic change detection.
 * Provides path-based access to nested tables and expression evaluation.
 *
 * Key functions:
 * - LoadStateFile() - Load Lua state file, initialize ExpressionCache (line 18, ~50 lines)
 * - GetValue() - Dot notation path access (line 71, ~10 lines)
 * - SetValue() - Update state + mark dirty (templated in header)
 * - EvaluateCondition() - Lua expression → bool via ExpressionCache (line 81, ~70 lines)
 * - EvaluateAsString() - Lua expression → string via ExpressionCache (line 152, ~90 lines)
 * - NavigatePath() - Traverse nested tables by dot path (line 241, ~35 lines)
 *
 * State file format: Lua file returns table with `data` and optional `computed` sections
 * Example: return { data = { fps = 60 }, computed = { ... } }
 *
 * Integration:
 * - ExpressionCache: All expression evaluation goes through cache (99.8% hit rate)
 * - TemplateParser: Calls EvaluateCondition/AsString for v-if and {{ }} directives
 * - ReactiveUI: Checks IsDirty() to skip unnecessary re-renders
 */

#include "systems/LuaUIState.h"
#include "util/Logger.h"
#include "controllers/ScriptManager.h"
#include <sstream>
#include <vector>

LuaUIState::LuaUIState()
    : m_lua(nullptr), m_isDirty(true), m_isReady(false)
{
    // Get reference to ScriptManager's Lua VM
    ScriptManager &scriptMgr = ScriptManager::GetInstance();
    m_lua = &scriptMgr.GetLuaState();

    // Initialize expression cache
    m_exprCache = std::make_unique<ExpressionCache>();
    m_exprCache->Initialize(m_lua);
}

bool LuaUIState::LoadStateFile(const std::string &path)
{
    if (!m_lua)
    {
        LOG_ERROR("[LuaUIState] Lua state not initialized");
        return false;
    }

    try
    {
        LOG_INFO("[LuaUIState] Loading state file: {}", path);

        // Execute the Lua file which should return a table
        sol::load_result loadResult = m_lua->load_file(path);

        if (!loadResult.valid())
        {
            sol::error err = loadResult;
            LOG_ERROR("[LuaUIState] Failed to load file: {}", err.what());
            return false;
        }

        // Call the loaded chunk to get the returned table
        sol::protected_function_result result = loadResult();

        if (!result.valid())
        {
            sol::error err = result;
            LOG_ERROR("[LuaUIState] Failed to execute file: {}", err.what());
            return false;
        }

        // The result should be a table
        if (!result[0].is<sol::table>())
        {
            LOG_ERROR("[LuaUIState] File did not return a table");
            return false;
        }

        m_stateTable = result[0];

        // Back-compat: allow templates/expressions to reference `data` fields without the `data.` prefix.
        // This is done by setting a metatable on the root state table that forwards missing lookups and
        // writes to the `data` table (while preserving any existing metatable behavior).
        try
        {
            sol::object dataObj = m_stateTable.raw_get<sol::object>("data");
            if (dataObj.valid() && dataObj.is<sol::table>())
            {
                sol::object metaObj = m_stateTable[sol::metatable_key];
                sol::table meta = (metaObj.valid() && metaObj.is<sol::table>()) ? metaObj.as<sol::table>() : m_lua->create_table();

                sol::object prevIndexObj = meta.raw_get<sol::object>("__index");
                sol::object prevNewIndexObj = meta.raw_get<sol::object>("__newindex");

                meta.set_function("__index", [prevIndexObj](sol::table t, sol::object key) -> sol::object
                {
                    sol::object dataInnerObj = t.raw_get<sol::object>("data");
                    if (dataInnerObj.valid() && dataInnerObj.is<sol::table>())
                    {
                        sol::table dataTable = dataInnerObj.as<sol::table>();

                        if (key.is<std::string>())
                        {
                            const std::string k = key.as<std::string>();
                            sol::object v = dataTable.raw_get<sol::object>(k);
                            if (v.valid() && v.get_type() != sol::type::lua_nil)
                            {
                                return v;
                            }
                        }
                        else if (key.is<int>())
                        {
                            int k = key.as<int>();
                            sol::object v = dataTable.raw_get<sol::object>(k);
                            if (v.valid() && v.get_type() != sol::type::lua_nil)
                            {
                                return v;
                            }
                        }
                    }

                    if (prevIndexObj.valid() && prevIndexObj.get_type() != sol::type::lua_nil)
                    {
                        if (prevIndexObj.get_type() == sol::type::table)
                        {
                            sol::table prevIndexTable = prevIndexObj.as<sol::table>();
                            if (key.is<std::string>())
                            {
                                return prevIndexTable[key.as<std::string>()];
                            }
                            if (key.is<int>())
                            {
                                return prevIndexTable[key.as<int>()];
                            }
                        }
                        else if (prevIndexObj.get_type() == sol::type::function)
                        {
                            sol::protected_function prevIndexFn = prevIndexObj.as<sol::protected_function>();
                            sol::protected_function_result r = prevIndexFn(t, key);
                            if (r.valid())
                            {
                                return r.get<sol::object>();
                            }
                        }
                    }

                    return sol::nil;
                });

                meta.set_function("__newindex", [prevNewIndexObj](sol::table t, sol::object key, sol::object value)
                {
                    bool wrote = false;
                    if (key.is<std::string>())
                    {
                        const std::string k = key.as<std::string>();
                        if (k != "data" && k != "methods" && k != "computed")
                        {
                            sol::object dataInnerObj = t.raw_get<sol::object>("data");
                            if (dataInnerObj.valid() && dataInnerObj.is<sol::table>())
                            {
                                sol::table dataTable = dataInnerObj.as<sol::table>();
                                dataTable[k] = value;
                                wrote = true;
                            }
                        }
                    }
                    else if (key.is<int>())
                    {
                        sol::object dataInnerObj = t.raw_get<sol::object>("data");
                        if (dataInnerObj.valid() && dataInnerObj.is<sol::table>())
                        {
                            sol::table dataTable = dataInnerObj.as<sol::table>();
                            dataTable[key.as<int>()] = value;
                            wrote = true;
                        }
                    }

                    if (wrote)
                    {
                        return;
                    }

                    if (prevNewIndexObj.valid() && prevNewIndexObj.get_type() != sol::type::lua_nil)
                    {
                        if (prevNewIndexObj.get_type() == sol::type::function)
                        {
                            sol::protected_function prevNewIndexFn = prevNewIndexObj.as<sol::protected_function>();
                            prevNewIndexFn(t, key, value);
                            return;
                        }
                        if (prevNewIndexObj.get_type() == sol::type::table)
                        {
                            sol::table prevNewIndexTable = prevNewIndexObj.as<sol::table>();
                            if (key.is<std::string>())
                            {
                                prevNewIndexTable[key.as<std::string>()] = value;
                                return;
                            }
                            if (key.is<int>())
                            {
                                prevNewIndexTable[key.as<int>()] = value;
                                return;
                            }
                        }
                    }

                    // Default: raw set on the state table (avoid recursion into __newindex).
                    if (key.is<std::string>())
                    {
                        t.raw_set(key.as<std::string>(), value);
                    }
                    else if (key.is<int>())
                    {
                        t.raw_set(key.as<int>(), value);
                    }
                });

                m_stateTable[sol::metatable_key] = meta;
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("[LuaUIState] Failed to set data fallback metatable: {}", e.what());
        }

        m_isReady = true;
        m_isDirty = true;

        LOG_INFO("[LuaUIState] State file loaded successfully");
        return true;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("[LuaUIState] Exception loading state file: {}", e.what());
        return false;
    }
}

sol::object LuaUIState::GetValue(const std::string &key)
{
    if (!m_isReady)
    {
        return sol::nil;
    }

    return NavigatePath(key);
}

bool LuaUIState::EvaluateCondition(const std::string &expression)
{
    if (!m_isReady || !m_lua)
    {
        return false;
    }

    // Use expression cache for fast evaluation
    if (m_exprCache && m_exprCache->IsInitialized())
    {
        uint32_t exprId = m_exprCache->GetOrCompile(expression);
        if (exprId != UINT32_MAX)
        {
            return m_exprCache->EvaluateAsBool(exprId, m_stateTable);
        }
    }

    // Fallback to uncached evaluation (should rarely happen)
    try
    {
        std::string luaCode = "return function() return " + expression + " end";

        sol::load_result loadResult = m_lua->load(luaCode);
        if (!loadResult.valid())
        {
            sol::error err = loadResult;
            LOG_ERROR("[LuaUIState] Failed to load expression: {}", err.what());
            return false;
        }

        sol::protected_function func = loadResult();
        sol::environment env(*m_lua, sol::create, m_stateTable);
        sol::set_environment(env, func);

        sol::protected_function_result result = func();

        if (!result.valid())
        {
            sol::error err = result;
            LOG_ERROR("[LuaUIState] Failed to evaluate expression: {}", err.what());
            return false;
        }

        sol::object obj = result[0];

        if (obj.is<bool>())
        {
            return obj.as<bool>();
        }
        else if (obj.is<int>() || obj.is<double>())
        {
            return obj.as<double>() != 0.0;
        }
        else if (obj.is<std::string>())
        {
            return !obj.as<std::string>().empty();
        }
        else if (!obj.valid() || obj.get_type() == sol::type::lua_nil)
        {
            return false;
        }

        return true;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("[LuaUIState] Exception evaluating condition: {}", e.what());
        return false;
    }
}

std::string LuaUIState::EvaluateAsString(const std::string &expression)
{
    if (!m_isReady || !m_lua)
    {
        LOG_WARNING("[LuaUIState] Cannot evaluate '{}': state not ready", expression);
        return "";
    }

    // Use expression cache for fast evaluation
    if (m_exprCache && m_exprCache->IsInitialized())
    {
        uint32_t exprId = m_exprCache->GetOrCompile(expression);
        if (exprId != UINT32_MAX)
        {
            return m_exprCache->EvaluateAsString(exprId, m_stateTable);
        }
    }

    // Fallback to uncached evaluation (should rarely happen)
    try
    {
        std::string luaCode = "return function() return " + expression + " end";

        sol::load_result loadResult = m_lua->load(luaCode);
        if (!loadResult.valid())
        {
            sol::error err = loadResult;
            LOG_ERROR("[LuaUIState] Failed to load expression '{}': {}", expression, err.what());
            return "";
        }

        sol::protected_function func = loadResult();
        sol::environment env(*m_lua, sol::create, m_stateTable);
        sol::set_environment(env, func);

        sol::protected_function_result result = func();

        if (!result.valid())
        {
            sol::error err = result;
            LOG_ERROR("[LuaUIState] Failed to evaluate expression '{}': {}", expression, err.what());
            return "";
        }

        sol::object obj = result[0];

        if (obj.is<std::string>())
        {
            return obj.as<std::string>();
        }
        else if (obj.is<int>())
        {
            return std::to_string(obj.as<int>());
        }
        else if (obj.is<double>())
        {
            return std::to_string(obj.as<double>());
        }
        else if (obj.is<bool>())
        {
            return obj.as<bool>() ? "true" : "false";
        }
        else if (!obj.valid() || obj.get_type() == sol::type::lua_nil)
        {
            return "";
        }

        return "<object>";
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("[LuaUIState] Exception evaluating '{}' as string: {}", expression, e.what());
        return "";
    }
}

sol::object LuaUIState::NavigatePath(const std::string &path)
{
    if (!m_isReady)
    {
        return sol::nil;
    }

    // Split path by dots and navigate through tables.
    // Important: a leaf value being nil is not always an error (e.g., "data.selectedEntityId" when nothing is selected).
    // Only treat missing intermediate keys as errors.
    std::vector<std::string> parts;
    parts.reserve(4);

    {
        std::istringstream iss(path);
        std::string key;
        while (std::getline(iss, key, '.'))
        {
            parts.push_back(key);
        }
    }

    sol::object current = m_stateTable;
    for (size_t i = 0; i < parts.size(); i++)
    {
        const std::string &key = parts[i];

        if (!current.is<sol::table>())
        {
            return sol::nil;
        }

        sol::table table = current.as<sol::table>();
        current = table[key];

        if (!current.valid() || current.get_type() == sol::type::lua_nil)
        {
            if (i + 1 < parts.size())
            {
                LOG_ERROR("[LuaUIState] Key not found: {} in path: {}", key, path);
            }
            return sol::nil;
        }
    }

    return current;
}

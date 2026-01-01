/**
 * @file ScriptComponent.h
 * @brief Scripting behavior component supporting Lua and Python
 */

#pragma once

#include <string>
#include <sol/sol.hpp>
#include <pybind11/pybind11.h>
#include "util/Logger.h"

namespace py = pybind11;

/**
 * @brief Scripting language type discriminator
 */
enum class ScriptType
{
    Lua,    ///< Lua script (primary language for game logic)
    Python  ///< Python script (used for data analysis, rarely for gameplay)
};

/**
 * @brief Component for attaching scripted behaviors to entities
 *
 * Allows entities to have dynamic behavior defined in Lua or Python.
 * Scripts follow a lifecycle with callbacks: ready(), update(), destroy().
 *
 * **Script Lifecycle (Lua example):**
 * @code
 * -- res/scripts/MyBehavior.lua
 * MyBehavior = {
 *     data = { health = 100 }
 * }
 *
 * function MyBehavior:ready()
 *     -- Called once when component is created
 *     print("Script initialized")
 * end
 *
 * function MyBehavior:update(deltaTime)
 *     -- Called every frame by ScriptSystem
 *     self.data.health = self.data.health - deltaTime
 * end
 *
 * function MyBehavior:destroy()
 *     -- Called when entity is destroyed
 * end
 *
 * return MyBehavior
 * @endcode
 *
 * **Attaching to Entity:**
 * @code
 * // Load script and create instance
 * sol::state& lua = scriptManager.GetLuaState();
 * lua.script_file("../res/scripts/MyBehavior.lua");
 * sol::table instance = lua["MyBehavior"];
 *
 * // Attach to entity
 * ScriptComponent sc("MyBehavior", instance);
 * registry.RegisterComponent(entityId, sc);
 * @endcode
 *
 * **C++ Bindings Access:**
 * Scripts can access C++ functions bound in ScriptManager:
 * - Registry functions (CreateEntity, GetComponent, etc.)
 * - Transform manipulation
 * - Input queries
 * - Math utilities (glm types exposed)
 *
 * @note Lua is preferred for game logic (lighter, faster iteration)
 * @note Python is used for data analysis and build tools
 * @see ScriptSystem for lifecycle management
 * @see ScriptManager for C++ ↔ script bindings
 */
struct ScriptComponent
{
    std::string Name;  ///< Script identifier/class name
    ScriptType Type;   ///< Language discriminator (Lua or Python)

    sol::table LuaClass;     ///< Lua script instance (valid if Type == Lua)
    py::object PythonClass;  ///< Python script instance (valid if Type == Python)

    /**
     * @brief Construct Lua script component
     * @param name Script identifier
     * @param luaClass Lua table instance containing script methods
     * @note Automatically calls ready() callback if present
     */
    ScriptComponent(std::string name, sol::table luaClass)
        : Name(name), Type(ScriptType::Lua), LuaClass(luaClass)
    {
        sol::function ready = luaClass["ready"];
        try
        {
            ready(luaClass);
        }
        catch (const sol::error &e)
        {
            LOG_ERROR("Error calling Lua Ready: {}", e.what());
        }
    }

    /**
     * @brief Construct Python script component
     * @param name Script identifier
     * @param pythonClass Python object instance
     * @note Python ready() must be called manually (not auto-invoked)
     */
    ScriptComponent(std::string name, py::object pythonClass)
        : Name(name), Type(ScriptType::Python), PythonClass(pythonClass)
    {
    }

    /**
     * @brief Cleanup script resources
     * @note Lua tables are abandoned to prevent premature GC
     */
    ~ScriptComponent()
    {
        if (Type == ScriptType::Lua)
        {
            LuaClass.abandon();
        }
    }
};

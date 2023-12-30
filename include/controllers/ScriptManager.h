#pragma once

#include "util/debug.h"

#ifdef USE_LUA_SCRIPTING
#include <sol/sol.hpp>
namespace LuaBindings
{
    void RegisterEnums(sol::state &lua);
    void RegisterTypes(sol::state &lua);
    void RegisterFunctions(sol::state &lua);
}
#endif

#ifdef USE_PYTHON_SCRIPTING
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>

namespace py = pybind11;
#endif

#include <iostream>

static const std::string &EVENT_QUEUE = "event_queue";
static const std::string &HANDLE_INPUT_F = "handle_input";

class ScriptManager
{
public:
    static ScriptManager &GetInstance()
    {
        static ScriptManager instance;
        return instance;
    };

    ScriptManager(ScriptManager const &) = delete;
    void operator=(ScriptManager const &) = delete;

    void Initialize();
    void Shutdown();
    void Run(const std::string &scriptSrc);

#ifdef USE_LUA_SCRIPTING

    void CreateList(const std::string &key)
    {
        sol::table targetTable = lua.create_table();
        lua[key] = targetTable;
    };

    template <typename T>
    void AddToTable(const std::string &tableName, const T &value)
    {
        sol::table table = lua[tableName];
        if (!table.valid())
        {
            std::cerr << "Table " << tableName << " not found in Lua" << std::endl;
            return;
        }
        table.add(value);
    };

    template <typename T>
    void SetScriptVar(const std::string &key, const T &value)
    {
        lua[key] = value;
    };

    sol::table GetLuaTable(const std::string &className)
    {
        return lua[className];
    };

#endif

#ifdef USE_PYTHON_SCRIPTING
    void CreateList(const std::string &key);

    template <typename T>
    void AddToTable(const std::string &name, const T &value)
    {
        try
        {
            std::cout << "appending to " << name << std::endl;
            py::object global_namespace = py::globals();
            py::list py_list = global_namespace[name.c_str()].cast<py::list>();

            std::cout << "accessed list" << std::endl;

            py_list.append(value);

            std::cout << "Success" << std::endl;
        }
        catch (py::error_already_set &e)
        {
            std::cerr << "Error in accessing or modifying Python list '"
                      << name << "': " << e.what() << std::endl;
        }
    }

    py::object ImportModule(const std::string &moduleName)
    {
        // Check if the module is already imported
        py::dict sys_modules = py::module::import("sys").attr("modules").cast<py::dict>();
        if (sys_modules.contains(moduleName.c_str()))
        {
            // Module is already imported, return the existing module
            return sys_modules[moduleName.c_str()];
        }
        else
        {
            // Module is not imported yet, import and return the module
            return py::module::import(moduleName.c_str());
        }
    }

    void SetClassAttribute(const std::string &className, const std::string &attribute, const std::string &value)
    {
        try
        {
            py::object scriptClass = ImportModule(className);
            scriptClass.attr(attribute.c_str()) = value;
        }
        catch (const py::error_already_set &e)
        {
            std::cerr << "Python error: " << e.what() << std::endl;
        }
    }
#endif

    void ProcessInput();

private:
    ScriptManager(){};

#ifdef USE_LUA_SCRIPTING
    sol::state lua;
#endif
#ifdef USE_PYTHON_SCRIPTING
    std::unique_ptr<py::scoped_interpreter> guard;
#endif
};

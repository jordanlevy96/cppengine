#pragma once

#include "util/debug.h"
#include <sol/sol.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <iostream>

namespace py = pybind11;

static const std::string &EVENT_QUEUE = "EventQueue";
static const std::string &HANDLE_INPUT_F = "HandleInput";

namespace LuaBindings
{
    void RegisterEnums(sol::state &lua);
    void RegisterTypes(sol::state &lua);
    void RegisterFunctions(sol::state &lua);
}

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
    void CreateList(const std::string &key);

    // Lua-specific methods
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

    // Python-specific methods
    template <typename T>
    void AddToList(const std::string &name, const T &value)
    {
        try
        {
            py::object global_namespace = py::globals();
            py::object py_obj = global_namespace[name.c_str()];

            if (py_obj.is_none())
            {
                throw std::runtime_error("List not found in the global namespace");
            }
            if (!py::isinstance<py::list>(py_obj))
            {
                throw std::runtime_error("Object is not a list");
            }

            py::list py_list = py_obj.cast<py::list>();
            py_list.append(value);
        }
        catch (const py::error_already_set &e)
        {
            std::cerr << "Error in accessing or modifying Python list '" << name << "': " << e.what() << std::endl;
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Runtime error: " << e.what() << std::endl;
        }
    }

    template <typename T>
    void AddToList(const std::string &name, const T &value, const std::string &module)
    {
        try
        {
            py::object module_namespace = ImportModule(module.c_str());
            py::object py_obj = module_namespace.attr(name.c_str());

            if (py_obj.is_none())
            {
                throw std::runtime_error("List not found in the global namespace");
            }
            if (!py::isinstance<py::list>(py_obj))
            {
                throw std::runtime_error("Object is not a list");
            }

            py::list py_list = py_obj.cast<py::list>();
            py_list.append(value);
        }
        catch (const py::error_already_set &e)
        {
            std::cerr << "Error in accessing or modifying Python list '" << name << "': " << e.what() << std::endl;
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Runtime error: " << e.what() << std::endl;
        }
    }

    py::object ImportModule(const std::string &moduleName);

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

    void ProcessInput();

private:
    ScriptManager(){};

    sol::state lua;
    std::unique_ptr<py::scoped_interpreter> guard;
};

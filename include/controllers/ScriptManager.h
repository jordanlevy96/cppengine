#pragma once

#include <sol/sol.hpp>

const std::string EVENT_QUEUE = "eventQueue";
const std::string HANDLE_INPUT_F = "HandleInput";

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
    void Run(const char *scriptSrc);

    void CreateTable(const std::string &key)
    {
        sol::table targetTable = lua.create_table();
        lua[key] = targetTable;
    }

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
    }

    template <typename T>
    void SetScriptVar(const std::string &key, const T &value)
    {
        lua[key] = value;
    }

    void ProcessInput();

private:
    ScriptManager(){};
    sol::state lua;
};

namespace LuaBindings
{
    void RegisterEnums(sol::state &lua);
    void RegisterTypes(sol::state &lua);
    void RegisterFunctions(sol::state &lua);
}
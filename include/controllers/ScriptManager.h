#pragma once

#include <sol/sol.hpp>

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
    template <typename T>
    void SetScriptVar(const std::string &key, const T &value);

private:
    ScriptManager(){};
    sol::state lua;
};

namespace LuaBindings
{
    void RegisterEnums(sol::state &lua);
    void RegisterTypes(sol::state &lua);
}
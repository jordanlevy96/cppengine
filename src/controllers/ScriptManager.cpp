#include "Camera.h"
#include "controllers/Game.h"
#include "controllers/Registry.h"
#include "controllers/ScriptManager.h"
#include "util/Uniform.h"

// Generic engine bindings
#include "components/Tween.h"
#include "components/Lighting.h"
#include "util/SceneTraversal.h"
#include "util/TransformUtils.h"
#include "systems/ReactiveUI.h"
#include "systems/HTMLRendererMT.h"

#include "controllers/WindowManager.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <variant>

// ============================================================================
// Lua Scripting
// ============================================================================

void ScriptManager::Run(const std::string &scriptSrc)
{
    // Run Lua script file (.lua files)
    lua.script_file(scriptSrc);
}

void ScriptManager::CreateList(const std::string &key)
{
    sol::table targetTable = lua.create_table();
    lua[key] = targetTable;
    LOG_DEBUG("Created new Lua table with key '{}'", key);
}

void ScriptManager::AddInputEventToQueue(const InputEvent &event)
{
    LOG_WARNING("Adding input event to Lua queue");
    LOG_WARNING("Event type: {}", static_cast<int>(event.type));
    sol::table luaEvent = lua.create_table();
    luaEvent["type"] = event.type;
    luaEvent["mods"] = event.mods;

    // Convert std::variant to appropriate Lua type
    std::visit([&](auto &&arg)
               {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>) {
            luaEvent["input"] = arg;
        } else if constexpr (std::is_same_v<T, glm::vec2>) {
            sol::table v = lua.create_table();
            v["x"] = arg.x;
            v["y"] = arg.y;
            luaEvent["input"] = v;
        } else if constexpr (std::is_same_v<T, glm::vec3>) {
            sol::table v = lua.create_table();
            v["x"] = arg.x;
            v["y"] = arg.y;
            v["z"] = arg.z;
            luaEvent["input"] = v;
        } }, event.input);

    sol::table queue = lua["EventQueue"];
    if (!queue.valid())
    {
        LOG_ERROR("[ScriptManager] EventQueue not found in Lua");
        return;
    }
    queue.add(luaEvent);
    LOG_DEBUG("[ScriptManager] Added input event to Lua EventQueue");
}

void ScriptManager::ProcessInput()
{
    Game &game = Game::GetInstance();
    GLFWwindow *window = WindowManager::GetInstance().window;

    // Call Lua input handler for game-specific input
    sol::function handleInputFunction = lua[HANDLE_INPUT_F];

    if (!handleInputFunction.valid())
    {
        LOG_ERROR("[ScriptManager] HandleInput function is not valid!");
        return;
    }

    try
    {
        handleInputFunction();
    }
    catch (const sol::error &e)
    {
        LOG_ERROR("[ScriptManager] Error calling HandleInput: {}", e.what());
    }
}

namespace LuaBindings
{
    void RegisterEnums(sol::state &lua)
    {
        lua.new_enum("InputTypes",
                     "KEY", InputTypes::Key,
                     "CLICK", InputTypes::Click,
                     "CURSOR", InputTypes::Cursor,
                     "RESIZE", InputTypes::Resize,
                     "SCROLL", InputTypes::Scroll);

        lua.new_enum("CameraDirections",
                     "FORWARD", CameraDirections::FORWARD,
                     "BACK", CameraDirections::BACK,
                     "LEFT", CameraDirections::LEFT,
                     "RIGHT", CameraDirections::RIGHT);

        lua.new_enum("TransitionType",
                     "LINEAR", TransitionType::TRANS_LINEAR,
                     "SINE", TransitionType::TRANS_SINE);
    }

    void RegisterTypes(sol::state &lua)
    {
        lua.new_usertype<glm::vec2>("vec2",
                                    sol::call_constructor, sol::constructors<glm::vec2(float, float)>(),
                                    "x", &glm::vec2::x,
                                    "y", &glm::vec2::y);

        lua.new_usertype<glm::vec3>("vec3",
                                    sol::call_constructor, sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
                                    "x", &glm::vec3::x,
                                    "y", &glm::vec3::y,
                                    "z", &glm::vec3::z);

        lua.new_usertype<Transform>("Transform",
                                    "Pos", &Transform::Pos,
                                    "Color", &Transform::Color,
                                    "Scale", &Transform::Scale,
                                    "Rotation", &Transform::Rotation);

        lua.new_usertype<Tween>("Tween",
                                "Start", &Tween::Start,
                                "End", &Tween::End,
                                "Duration", &Tween::Duration,
                                "elapsed", &Tween::elapsed,
                                "isActive", &Tween::isActive);

        // The camera should be constructed before initializing Lua
        lua.new_usertype<Camera>("Camera",
                                 "transform", &Camera::transform,
                                 "fov", &Camera::fov,
                                 "front", &Camera::front,
                                 // casting is necessary here because the function is overloaded, which Lua does not support
                                 "SetPerspective", std::function<void(Camera *, float)>(static_cast<void (Camera::*)(float)>(&Camera::SetPerspective)),
                                 "Move", &Camera::Move,
                                 "RotateByMouse", &Camera::RotateByMouse);

        lua.new_usertype<Game>("Game",
                               "GetInstance", &Game::GetInstance,
                               "delta", &Game::delta,
                               "registry", &Game::registry,
                               "camera", &Game::cam,
                               "conf", &Game::conf,
                               "window", &Game::windowManager,
                               "htmlRenderer", &Game::htmlRenderer,
                               "StartGame", &Game::StartGame,
                               "ResetGame", &Game::ResetGame,
                               "ReturnToMainMenu", &Game::ReturnToMainMenu);

        lua.new_usertype<HTMLRendererMT>("HTMLRendererMT",
                                         "HandleClickEvent", &HTMLRendererMT::HandleClickEvent,
                                         "UpdateHoverState", &HTMLRendererMT::UpdateHoverState);

        lua.new_usertype<Config>("Config",
                                 "resPath", &Config::ResourcePath);

        lua.new_usertype<WindowManager>("Window",
                                        "CloseWindow", &WindowManager::CloseWindow,
                                        "GetSize", &WindowManager::GetSize);

        lua.new_usertype<Registry>("Registry",
                                   "GetInstance", &Registry::GetInstance,
                                   "GetEntityByName", &Registry::GetEntityByName);

        lua.new_usertype<InputEvent>(
            "InputEvent",
            "type", &InputEvent::type,
            "input", &InputEvent::input,
            "mods", &InputEvent::mods);
    }

    void RegisterFunctions(sol::state &lua)
    {
        lua.set_function("CreateRenderComponent", &Registry::CreateRenderComponent);
        lua.set_function("AttachScript", sol::resolve<void(EntityID, const std::string &, sol::table)>(&Registry::AttachScript));
        lua.set_function("CreateCube", &Registry::CreateCube);

        // ====================================================================
        // GENERIC ENGINE BINDINGS - Entity & Component Management
        // ====================================================================

        // UI State Update
        lua.set_function("UpdateGameUI", [](int score, int lines, int level, const std::string &nextPiece)
                         {
            ReactiveUI& reactiveUI = ReactiveUI::GetInstance();
            auto luaState = reactiveUI.GetLuaState();
            if (luaState) {
                luaState->SetValue("data.score", score);
                luaState->SetValue("data.lines", lines);
                luaState->SetValue("data.level", level);
                luaState->SetValue("data.nextPiece", nextPiece);
            } });

        lua.set_function("UpdateGameOver", [](int finalScore)
                         {
            std::cout << "GAME OVER - Final Score: " << finalScore << std::endl;
            ReactiveUI& reactiveUI = ReactiveUI::GetInstance();
            auto luaState = reactiveUI.GetLuaState();

            if (luaState) {
                luaState->SetValue("data.gameOver", true);
                luaState->SetValue("data.finalScore", finalScore);
            } });

        // Entity Management
        lua.set_function("RegisterEntity", sol::overload(
                                               []()
                                               { return Registry::GetInstance().RegisterEntity(); },
                                               [](EntityID parent)
                                               { return Registry::GetInstance().RegisterEntity(parent); }));

        lua.set_function("DestroyEntity", [](EntityID id)
                         { Registry::GetInstance().DestroyEntity(id); });

        // Hierarchy Operations
        lua.set_function("AddChild", &AddChild);
        lua.set_function("GetParent", &GetParent);
        lua.set_function("RemoveChild", &RemoveChild);

        // Component Access
        lua.set_function("GetTransform", [](EntityID id) -> Transform &
                         { return Registry::GetInstance().GetComponent<Transform>(id); });

        lua.set_function("GetTween", [](EntityID id) -> Tween &
                         { return Registry::GetInstance().GetComponent<Tween>(id); });

        // Component Registration
        lua.set_function("RegisterRenderComponent", [](EntityID id, std::shared_ptr<RenderComponent> rc)
                         { Registry::GetInstance().RegisterComponent<RenderComponent>(id, *rc); });

        lua.set_function("RegisterLighting", [](EntityID id, EntityID lightID)
                         {
            Lighting lightComp = Lighting(lightID);
            Registry::GetInstance().RegisterComponent<Lighting>(id, lightComp); });

        // Transform Operations
        lua.set_function("TranslateEntity", &TransformUtils::translate);
        lua.set_function("RotateEntity", &TransformUtils::rotate);

        // Tween Component Creation (creates tween with C++ move_to function)
        lua.set_function("CreateTweenComponent", [](EntityID id, float duration)
                         {
            Transform& transform = Registry::GetInstance().GetComponent<Transform>(id);
            Tween tween = Tween(&TransformUtils::move_to,
                               transform.Pos,
                               transform.Pos + glm::vec3(0, -2, 0),
                               duration,
                               TransitionType::TRANS_LINEAR);
            Registry::GetInstance().RegisterComponent<Tween>(id, tween); });

        // Utility
        lua.set_function("GetEntityByName", [](const std::string &name) -> EntityID
                         { return Registry::GetInstance().GetEntityByName(name); });
    }
}

// ============================================================================
// Python Scripting
// ============================================================================

py::object ScriptManager::ImportModule(const std::string &moduleName)
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

// ============================================================================
// Initialization & Shutdown - Lua and Python
// ============================================================================

void ScriptManager::Initialize()
{
    LOG_TRACE_L3("Initializing ScriptManager...");
    // Initialize Lua
    LuaBindings::RegisterEnums(lua);
    LuaBindings::RegisterTypes(lua);
    LuaBindings::RegisterFunctions(lua);
    lua.open_libraries(sol::lib::base, sol::lib::table, sol::lib::os, sol::lib::math, sol::lib::string, sol::lib::debug);

    // Override Lua's print function to route through C++ logging with source location
    lua.set_function("print", [](sol::variadic_args args, sol::this_state s)
                     {
        sol::state_view lua(s);

        // Get caller's source location using debug.getinfo
        std::string source = "?";
        int line = 0;
        sol::function getinfo = lua["debug"]["getinfo"];
        if (getinfo.valid()) {
            sol::table info = getinfo(2, "Sl"); // level 2 = caller, "Sl" = source + line
            if (info.valid()) {
                sol::optional<std::string> src = info["short_src"];
                sol::optional<int> ln = info["currentline"];
                if (src) source = *src;
                if (ln) line = *ln;
            }
        }

        // Build output string
        std::string output;
        bool first = true;
        for (auto arg : args) {
            if (!first) output += "\t";
            first = false;
            sol::function tostring = lua["tostring"];
            sol::object result = tostring(arg);
            if (result.is<std::string>()) {
                output += result.as<std::string>();
            } else {
                output += "<unprintable>";
            }
        }
        LOG_INFO("[Lua] {}:{} - {}", source, line, output); });

    CreateList(EVENT_QUEUE);

    LOG_TRACE_L1("Lua: Running init.lua");
    Run(Game::GetInstance().conf.ResourcePath + "scripts/init.lua");

    // Initialize Python
    guard = std::make_unique<py::scoped_interpreter>();
    LOG_TRACE_L1("Python: Running init.py");
    std::ifstream file(Game::GetInstance().conf.ResourcePath + "scripts/init.py");
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open Python init script");
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string script_content = buffer.str();
    py::exec(script_content, py::globals());

    LOG_INFO("INIT - ScriptManager: SUCCESS");
}

void ScriptManager::Shutdown()
{
    // Shutdown Lua
    lua["GameManager"] = sol::lua_nil;
    lua.collect_garbage();

    // Shutdown Python
    try
    {
        // Clear globals
        py::dict globals = py::globals();
        std::vector<std::string> keys_to_delete;

        // Collect all keys; can't modify dict while iterating over it
        for (auto item : globals)
        {
            std::string key = py::str(item.first).cast<std::string>();
            keys_to_delete.push_back(key);
        }

        for (const auto &key : keys_to_delete)
        {
            globals.attr("pop")(key, py::none());
        }
    }
    catch (const py::error_already_set &e)
    {
        LOG_ERROR("Error clearing Python globals: {}", e.what());
    }
    guard.reset(); // End the Python interpreter

    LOG_INFO("SHUTDOWN - Python");
}

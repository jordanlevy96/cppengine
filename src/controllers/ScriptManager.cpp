#include "Camera.h"
#include "controllers/App.h"
#include "controllers/Registry.h"
#include "controllers/ScriptManager.h"
#include "util/Uniform.h"

// Generic engine bindings
#include "components/Tween.h"
#include "components/Lighting.h"
#include "util/SceneTraversal.h"
#include "util/TransformUtils.h"
#include "systems/ReactiveUI.h"

#include <iostream>
#include <fstream>
#include <sstream>

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
}

void ScriptManager::ProcessInput()
{
    App& app = App::GetInstance();
    GLFWwindow* window = WindowManager::GetInstance().window;

    // Static variables for key debouncing
    static bool speedKeyPressed = false;

    // Speed controls (only in VARIABLE mode)
    if (app.GetGameMode() == GameMode::VARIABLE) {
        bool anySpeedKeyPressed = false;

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            if (!speedKeyPressed) {
                app.SetSimulationSpeed(SimulationSpeed::PAUSED);
                std::cout << "[Speed] PAUSED" << std::endl;
            }
            anySpeedKeyPressed = true;
        } else if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            if (!speedKeyPressed) {
                app.SetSimulationSpeed(SimulationSpeed::NORMAL);
                std::cout << "[Speed] NORMAL (1x)" << std::endl;
            }
            anySpeedKeyPressed = true;
        } else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            if (!speedKeyPressed) {
                app.SetSimulationSpeed(SimulationSpeed::FAST);
                std::cout << "[Speed] FAST (2x)" << std::endl;
            }
            anySpeedKeyPressed = true;
        } else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
            if (!speedKeyPressed) {
                app.SetSimulationSpeed(SimulationSpeed::FASTER);
                std::cout << "[Speed] FASTER (3x)" << std::endl;
            }
            anySpeedKeyPressed = true;
        } else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
            if (!speedKeyPressed) {
                app.SetSimulationSpeed(SimulationSpeed::FASTEST);
                std::cout << "[Speed] FASTEST (5x)" << std::endl;
            }
            anySpeedKeyPressed = true;
        } else if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {
            if (!speedKeyPressed) {
                app.SetSimulationSpeed(SimulationSpeed::UNCAPPED);
                std::cout << "[Speed] UNCAPPED (MAX)" << std::endl;
            }
            anySpeedKeyPressed = true;
        }

        speedKeyPressed = anySpeedKeyPressed;
    }

    // Call Lua input handler for game-specific input
    sol::function handleInputFunction = lua[HANDLE_INPUT_F];
    try
    {
        handleInputFunction();
    }
    catch (const sol::error &e)
    {
        std::cerr << "Error calling HandleInput: " << e.what() << std::endl;
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

        lua.new_usertype<Camera>("Camera",
                                 "transform", &Camera::transform,
                                 "fov", &Camera::fov,
                                 "front", &Camera::front,
                                 // casting is necessary here because the function is overloaded, which Lua does not support
                                 "SetPerspective", std::function<void(Camera *, float)>(static_cast<void (Camera::*)(float)>(&Camera::SetPerspective)),
                                 "Move", &Camera::Move,
                                 "RotateByMouse", &Camera::RotateByMouse);

        lua.new_usertype<App>("App",
                              "GetInstance", &App::GetInstance,
                              "delta", &App::delta,
                              "registry", &App::registry,
                              "camera", &App::cam,
                              "conf", &App::conf,
                              "window", &App::windowManager,
                              "StartGame", &App::StartGame);

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
            "input", &InputEvent::input);
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
        lua.set_function("UpdateGameUI", [](int score, int lines, int level, const std::string& nextPiece) {
            ReactiveUI& reactiveUI = ReactiveUI::GetInstance();
            auto luaState = reactiveUI.GetLuaState();
            if (luaState) {
                luaState->SetValue("data.score", score);
                luaState->SetValue("data.lines", lines);
                luaState->SetValue("data.level", level);
                luaState->SetValue("data.nextPiece", nextPiece);
            }
        });

        // Entity Management
        lua.set_function("RegisterEntity", sol::overload(
            []() { return Registry::GetInstance().RegisterEntity(); },
            [](EntityID parent) { return Registry::GetInstance().RegisterEntity(parent); }
        ));

        // Hierarchy Operations
        lua.set_function("AddChild", &AddChild);
        lua.set_function("GetParent", &GetParent);

        // Component Access
        lua.set_function("GetTransform", [](EntityID id) -> Transform& {
            return Registry::GetInstance().GetComponent<Transform>(id);
        });

        lua.set_function("GetTween", [](EntityID id) -> Tween& {
            return Registry::GetInstance().GetComponent<Tween>(id);
        });

        // Component Registration
        lua.set_function("RegisterRenderComponent", [](EntityID id, std::shared_ptr<RenderComponent> rc) {
            Registry::GetInstance().RegisterComponent<RenderComponent>(id, *rc);
        });

        lua.set_function("RegisterLighting", [](EntityID id, EntityID lightID) {
            Lighting lightComp = Lighting(lightID);
            Registry::GetInstance().RegisterComponent<Lighting>(id, lightComp);
        });

        // Transform Operations
        lua.set_function("TranslateEntity", &TransformUtils::translate);
        lua.set_function("RotateEntity", &TransformUtils::rotate);

        // Tween Component Creation (creates tween with C++ move_to function)
        lua.set_function("CreateTweenComponent", [](EntityID id, float duration) {
            Transform& transform = Registry::GetInstance().GetComponent<Transform>(id);
            Tween tween = Tween(&TransformUtils::move_to,
                               transform.Pos,
                               transform.Pos + glm::vec3(0, -2, 0),
                               duration,
                               TransitionType::TRANS_LINEAR);
            Registry::GetInstance().RegisterComponent<Tween>(id, tween);
        });

        // Utility
        lua.set_function("GetEntityByName", [](const std::string& name) -> EntityID {
            return Registry::GetInstance().GetEntityByName(name);
        });
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
// Initialization & Shutdown - Both Languages
// ============================================================================

void ScriptManager::Initialize()
{
    // Initialize Lua
    LuaBindings::RegisterEnums(lua);
    LuaBindings::RegisterTypes(lua);
    LuaBindings::RegisterFunctions(lua);
    lua.open_libraries(sol::lib::base, sol::lib::table, sol::lib::os, sol::lib::math);
    CreateList(EVENT_QUEUE);
    Run(App::GetInstance().conf.ResourcePath + "scripts/init.lua");

    // Initialize Python
    guard = std::make_unique<py::scoped_interpreter>();
    std::cout << "Python: Running init.py" << std::endl;

    std::ifstream file(App::GetInstance().conf.ResourcePath + "scripts/init.py");
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open Python init script");
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string script_content = buffer.str();
    py::exec(script_content, py::globals());

    std::cout << "INIT - ScriptManager: SUCCESS" << std::endl;
}

void ScriptManager::Shutdown()
{
    // Shutdown Lua
    lua["GameManager"] = sol::lua_nil;
    lua.collect_garbage();

    // Shutdown Python
    std::cout << "SHUTDOWN - Python" << std::endl;
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
        std::cerr << "Error clearing Python globals: " << e.what() << std::endl;
    }
}

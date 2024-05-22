#pragma once

#include "Camera.h"
#include "controllers/WindowManager.h"
#include "controllers/Registry.h"
#include "systems/UI.h"

#ifdef USE_PYTHON_SCRIPTING
#include <pybind11/pybind11.h>
#endif

extern "C" void stbi_set_flip_vertically_on_load(int flag);

struct Config
{
    // default values can be overwritten via YAML
    float WindowWidth = 800;
    float WindowHeight = 600;
    float targetFPS = 60;
    std::string ResourcePath = "../res/";
};

class App
{
public:
    Config conf;
    Camera *cam;
    UI *ui;
    // time since last frame
    double delta = 0;
    Registry *registry;
    WindowManager *windowManager;
    ScriptManager *scriptManager;

    std::string test = "test string";

    static App &GetInstance()
    {
        static App instance;
        return instance;
    }

#ifdef USE_PYTHON_SCRIPTING
    static py::object GetPyInstance()
    {
        static py::object instance = py::cast(&GetInstance(), py::return_value_policy::reference);
        return instance;
    }
#endif

    App(App const &) = delete;
    void operator=(App const &) = delete;

    bool Initialize();
    void Shutdown();
    void Run();
    void CloseWindow();

private:
    App(){};
    bool LoadConfig(const std::string &configPath);
};
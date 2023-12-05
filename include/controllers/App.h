#pragma once

#include "Camera.h"
#include "components/RenderComponent.h"
#include "controllers/WindowManager.h"
#include "controllers/Registry.h"
#include "systems/UI.h"
#include "systems/ScriptSystem.h"
#include "systems/RenderSystem.h"

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
    double delta = 0;
    Registry *registry;
    WindowManager *windowManager;
    ScriptManager *lua;

    static App &GetInstance()
    {
        static App instance;
        return instance;
    }

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
#pragma once

#include "Camera.h"
#include "Tetrimino.h"
#include "components/RenderComponent.h"
#include "controllers/WindowManager.h"
#include "controllers/Registry.h"
#include "systems/UI.h"
#include "systems/RenderSystem.h"

#include "util/globals.h"

extern "C" void stbi_set_flip_vertically_on_load(int flag);

class App
{
public:
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
    bool LoadConfig();
};
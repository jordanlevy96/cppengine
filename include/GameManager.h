#pragma once

#include <WindowManager.h>
#include <Camera.h>
#include <GameObject.h>
#include <UI.h>

#include <vector>

extern "C" void stbi_set_flip_vertically_on_load(int flag);

class GameManager
{
public:
    static GameManager &GetInstance()
    {
        static GameManager instance;
        return instance;
    }

    GameManager(GameManager const &) = delete;
    void operator=(GameManager const &) = delete;

    bool Initialize();
    void Shutdown();

    void Run();
    WindowManager *windowManager;
    Camera *cam;
    UI *ui;
    std::vector<GameObject *> objects;
    double delta = 0;

private:
    GameManager(){};
};
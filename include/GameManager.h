#pragma once

#include <WindowManager.h>

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

    // Note: Scott Meyers mentions in his Effective Modern
    //       C++ book, that deleted functions should generally
    //       be public as it results in better error messages
    //       due to the compilers behavior to check accessibility
    //       before deleted status

    bool Initialize();

    void Run();
    WindowManager *windowManager;

private:
    GameManager(){};
};
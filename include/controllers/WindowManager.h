#pragma once

#include "controllers/ScriptManager.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <functional>

enum InputTypes
{
    Key,
    Click,
    Cursor,
    Resize,
    Scroll
};

struct InputEvent
{
    int type;
    std::string input;
};

class WindowManager
{
public:
    static WindowManager &GetInstance()
    {
        static WindowManager instance;
        return instance;
    }

    WindowManager(WindowManager const &) = delete;
    void operator=(WindowManager const &) = delete;

    bool Initialize(int const width, int const height);
    void Shutdown();

    GLFWwindow *window;

private:
    WindowManager(){};
    static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void click_callback(GLFWwindow *window, int button, int action, int mods);
    static void cursorPos_callback(GLFWwindow *window, double xpos, double ypos);
    static void resize_callback(GLFWwindow *window, int in_width, int in_height);
    static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
};

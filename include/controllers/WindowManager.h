#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <functional>

struct EventCallbacks
{
    std::function<void(GLFWwindow *window, int key, int scancode, int action, int mods)> keyCallback = nullptr;
    std::function<void(GLFWwindow *window, int button, int action, int mods)> clickCallback = nullptr;
    std::function<void(GLFWwindow *window, double xpos, double ypos)> cursorPosCallback = nullptr;
    std::function<void(GLFWwindow *window, int in_width, int in_height)> resizeCallback = nullptr;
    std::function<void(GLFWwindow *window, double xoffset, double yoffset)> scrollCallback = nullptr;
    EventCallbacks(
        std::function<void(GLFWwindow *, int, int, int, int)> key,
        std::function<void(GLFWwindow *, int, int, int)> click,
        std::function<void(GLFWwindow *, double, double)> cursor,
        std::function<void(GLFWwindow *, int, int)> resize,
        std::function<void(GLFWwindow *, double, double)> scroll)
        : keyCallback(key), clickCallback(click), cursorPosCallback(cursor), resizeCallback(resize), scrollCallback(scroll)
    {
    }
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
    void shutdown();

    void setEventCallbacks(EventCallbacks *callbacks);

    GLFWwindow *window;

protected:
    EventCallbacks *callbacks;

private:
    WindowManager(){};
    static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void click_callback(GLFWwindow *window, int button, int action, int mods);
    static void cursorPos_callback(GLFWwindow *window, double xpos, double ypos);
    static void resize_callback(GLFWwindow *window, int in_width, int in_height);
    static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
};

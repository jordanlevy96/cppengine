#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <functional>

struct EventCallbacks
{
    std::function<void(GLFWwindow *window, int key, int scancode, int action, int mods)> keyCallback = nullptr;
    std::function<void(GLFWwindow *window, int button, int action, int mods)> mouseCallback = nullptr;
    std::function<void(GLFWwindow *window, int in_width, int in_height)> resizeCallback = nullptr;
    EventCallbacks(
        std::function<void(GLFWwindow *, int, int, int, int)> key,
        std::function<void(GLFWwindow *, int, int, int)> mouse,
        std::function<void(GLFWwindow *, int, int)> resize)
        : keyCallback(key), mouseCallback(mouse), resizeCallback(resize)
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
    static void mouse_callback(GLFWwindow *window, int button, int action, int mods);
    static void resize_callback(GLFWwindow *window, int in_width, int in_height);
};

#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// This interface lets us write our own class that can be notified by input
// events, such as key presses and mouse movement.
class EventCallbacks
{

public:
    virtual void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) = 0;

    virtual void mouseCallback(GLFWwindow *window, int button, int action, int mods) = 0;

    virtual void resizeCallback(GLFWwindow *window, int in_width, int in_height) = 0;
};

class WindowManager
{
public:
    static WindowManager &GetInstance()
    {
        static WindowManager instance;
        return instance;
    }

    // WindowManager(WindowManager const &) = delete;
    // void operator=(WindowManager const &) = delete;

    bool Initialize(int const width, int const height);
    void shutdown();

    void setEventCallbacks(EventCallbacks *callbacks);

    GLFWwindow *window;

protected:
    EventCallbacks *callbacks = nullptr;
    static WindowManager *instance;

private:
    WindowManager(){};
    // GLFW3 expects C-style callbacks, but we want to be able to use C++-style
    // callbacks so that we can avoid global variables.
    static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void mouse_callback(GLFWwindow *window, int button, int action, int mods);
    static void resize_callback(GLFWwindow *window, int in_width, int in_height);
};

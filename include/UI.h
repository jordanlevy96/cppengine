#pragma once

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

class UI
{
public:
    static UI &GetInstance()
    {
        static UI instance;
        return instance;
    }

    UI(UI const &) = delete;
    void operator=(UI const &) = delete;

    void Initialize(GLFWwindow *window);
    void Shutdown();
    void RenderWindow();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool show_demo_window = true;
    bool show_another_window = false;

private:
    UI(){};
};

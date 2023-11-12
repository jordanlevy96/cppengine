#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

extern "C" struct nk_context;
extern "C" struct nk_glfw;
extern "C" struct nk_font_atlas;

class UI
{
public:
    UI(GLFWwindow *window);
    ~UI();
    struct nk_context *ctx;
    void RenderWindow();

private:
    GLFWwindow *win;
    struct nk_glfw *glfw;
    struct nk_font_atlas *atlas;
};

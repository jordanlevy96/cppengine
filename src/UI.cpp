#include "UI.h"
#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_BOOL
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_COMMAND_USERDATA
#define NK_UINT_DRAW_INDEX
#define NK_INCLUDE_STANDARD_IO
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_IMPLEMENTATION
#include <nuklear.h>
#include <nuklear_glfw_gl3.h>

#include <iostream>

UI::UI(GLFWwindow *window)
{
    std::cout << "Creating UI instance" << std::endl;
    glfw = new struct nk_glfw();
    ctx = nk_glfw3_init(glfw, window, NK_GLFW3_INSTALL_CALLBACKS);
    const struct nk_font_config conf = nk_font_config(14.0f);
    nk_glfw3_font_stash_begin(glfw, &atlas);
    struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../res/fonts/DroidSans.ttf", 14.0f, &conf);
    struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../res/fonts/Roboto-Regular.ttf", 14.0f, &conf);
    nk_glfw3_font_stash_end(glfw);
    nk_style_set_font(ctx, &droid->handle);
    win = window;
    std::cout << "UI Creation: Success" << std::endl;
}

UI::~UI()
{
    nk_glfw3_shutdown(glfw);
    delete glfw;
    delete atlas;
}

void UI::RenderWindow()
{
    nk_glfw3_new_frame(glfw);
    if (nk_begin(ctx, "Calculator", nk_rect(10, 10, 180, 250),
                 NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_MOVABLE))
    {
        static int set = 0, prev = 0, op = 0;
        static const char numbers[] = "789456123";
        static const char ops[] = "+-*/";
        static double a = 0, b = 0;
        static double *current = &a;

        size_t i = 0;
        int solve = 0;
        {
            int len;
            char buffer[256];
            nk_layout_row_dynamic(ctx, 35, 1);
            len = snprintf(buffer, 256, "%.2f", *current);
            nk_edit_string(ctx, NK_EDIT_SIMPLE, buffer, &len, 255, nk_filter_float);
            buffer[len] = 0;
            *current = atof(buffer);
        }

        nk_layout_row_dynamic(ctx, 35, 4);
        for (i = 0; i < 16; ++i)
        {
            if (i >= 12 && i < 15)
            {
                if (i > 12)
                    continue;
                if (nk_button_label(ctx, "C"))
                {
                    a = b = op = 0;
                    current = &a;
                    set = 0;
                }
                if (nk_button_label(ctx, "0"))
                {
                    *current = *current * 10.0f;
                    set = 0;
                }
                if (nk_button_label(ctx, "="))
                {
                    solve = 1;
                    prev = op;
                    op = 0;
                }
            }
            else if (((i + 1) % 4))
            {
                if (nk_button_text(ctx, &numbers[(i / 4) * 3 + i % 4], 1))
                {
                    *current = *current * 10.0f + numbers[(i / 4) * 3 + i % 4] - '0';
                    set = 0;
                }
            }
            else if (nk_button_text(ctx, &ops[i / 4], 1))
            {
                if (!set)
                {
                    if (current != &b)
                    {
                        current = &b;
                    }
                    else
                    {
                        prev = op;
                        solve = 1;
                    }
                }
                op = ops[i / 4];
                set = 1;
            }
        }
        if (solve)
        {
            if (prev == '+')
                a = a + b;
            if (prev == '-')
                a = a - b;
            if (prev == '*')
                a = a * b;
            if (prev == '/')
                a = a / b;
            current = &a;
            if (set)
                current = &b;
            b = 0;
            set = 0;
        }
    }
    nk_end(ctx);
    nk_glfw3_render(glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
}

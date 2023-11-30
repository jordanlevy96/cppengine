#include "controllers/WindowManager.h"

#include <iostream>

std::unordered_map<int, std::string> keyMap = {
    // WASD
    {GLFW_KEY_W, "W"},
    {GLFW_KEY_A, "A"},
    {GLFW_KEY_S, "S"},
    {GLFW_KEY_D, "D"},

    // Numbers
    {GLFW_KEY_0, "0"},
    {GLFW_KEY_1, "1"},
    // ... (Continue for all number keys)

    // Function Keys
    {GLFW_KEY_F1, "F1"},
    {GLFW_KEY_F2, "F2"},
    // ... (Continue for all function keys)

    // Arrow Keys
    {GLFW_KEY_UP, "ArrowUp"},
    {GLFW_KEY_DOWN, "ArrowDown"},
    {GLFW_KEY_LEFT, "ArrowLeft"},
    {GLFW_KEY_RIGHT, "ArrowRight"},

    // Special Keys
    {GLFW_KEY_ESCAPE, "Escape"},
    {GLFW_KEY_ENTER, "Enter"},
    {GLFW_KEY_TAB, "Tab"},
    {GLFW_KEY_BACKSPACE, "Backspace"},
    {GLFW_KEY_INSERT, "Insert"},
    {GLFW_KEY_DELETE, "Delete"},
    {GLFW_KEY_RIGHT_SHIFT, "RightShift"},
    {GLFW_KEY_LEFT_SHIFT, "LeftShift"},
    {GLFW_KEY_RIGHT_CONTROL, "RightControl"},
    {GLFW_KEY_LEFT_CONTROL, "LeftControl"},
    {GLFW_KEY_RIGHT_ALT, "RightAlt"},
    {GLFW_KEY_LEFT_ALT, "LeftAlt"},
    {GLFW_KEY_SPACE, "Space"},
    {GLFW_KEY_PAGE_UP, "PageUp"},
    {GLFW_KEY_PAGE_DOWN, "PageDown"},
    {GLFW_KEY_HOME, "Home"},
    {GLFW_KEY_END, "End"},
    {GLFW_KEY_CAPS_LOCK, "CapsLock"},
    {GLFW_KEY_SCROLL_LOCK, "ScrollLock"},
    {GLFW_KEY_NUM_LOCK, "NumLock"},
    {GLFW_KEY_PRINT_SCREEN, "PrintScreen"},
    {GLFW_KEY_PAUSE, "Pause"},

    // Numpad Keys
    {GLFW_KEY_KP_0, "Numpad0"},
    {GLFW_KEY_KP_1, "Numpad1"},
    // ... (Continue for all numpad keys)
};

#define GLFW_KEY(x) (keyMap.count(x) ? keyMap[x] : "Unknown")

void error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

bool WindowManager::Initialize(int const width, int const height)
{
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    std::cout << "INIT - GLFW: SUCCESS" << std::endl;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(width, height, "Game", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    std::cout << "INIT - GLAD: SUCCESS" << std::endl;

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    glfwSwapInterval(1); // Set vsync

    glViewport(0, 0, width, height);

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, click_callback);
    glfwSetCursorPosCallback(window, cursorPos_callback);
    glfwSetFramebufferSizeCallback(window, resize_callback);
    glfwSetScrollCallback(window, scroll_callback);

    return true;
}

void WindowManager::Shutdown()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void WindowManager::CloseWindow()
{
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

glm::vec2 WindowManager::GetSize()
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    return glm::vec2(width, height);
}

void WindowManager::key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        ScriptManager &sm = ScriptManager::GetInstance();
        InputEvent event;
        event.type = InputTypes::Key;
        event.input = GLFW_KEY(key);

        sm.AddToTable(EVENT_QUEUE, event);
    }
}

void WindowManager::click_callback(GLFWwindow *window, int button, int action, int mods)
{
    ScriptManager &sm = ScriptManager::GetInstance();
    InputEvent event;
    event.type = InputTypes::Click;
    event.input = "click";

    sm.AddToTable(EVENT_QUEUE, event);
}

void WindowManager::cursorPos_callback(GLFWwindow *window, double xpos, double ypos)
{
    ScriptManager &sm = ScriptManager::GetInstance();
    InputEvent event;
    event.type = InputTypes::Cursor;
    event.input = glm::vec2(xpos, ypos);

    sm.AddToTable(EVENT_QUEUE, event);
}

void WindowManager::resize_callback(GLFWwindow *window, int in_width, int in_height)
{
    ScriptManager &sm = ScriptManager::GetInstance();
    InputEvent event;
    event.type = InputTypes::Resize;
    event.input = glm::vec2(in_width, in_height);

    sm.AddToTable(EVENT_QUEUE, event);
}

void WindowManager::scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    ScriptManager &sm = ScriptManager::GetInstance();
    InputEvent event;
    event.type = InputTypes::Scroll;
    event.input = glm::vec2(xoffset, yoffset);

    sm.AddToTable(EVENT_QUEUE, event);
}

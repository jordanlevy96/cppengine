#include <WindowManager.h>
#include <iostream>

void error_callback(int error, const char *description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
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
    glfwSetMouseButtonCallback(window, mouse_callback);
    glfwSetFramebufferSizeCallback(window, resize_callback);

    return true;
}

void WindowManager::shutdown()
{
    delete callbacks;
    glfwDestroyWindow(window);
    glfwTerminate();
}

void WindowManager::setEventCallbacks(EventCallbacks *callbacks_in)
{
    callbacks = callbacks_in;
    std::cout << "Callbacks set!" << std::endl;
}

void WindowManager::key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    WindowManager *instance = &WindowManager::GetInstance();
    if (instance && instance->callbacks)
    {
        instance->callbacks->keyCallback(window, key, scancode, action, mods);
    }
}

void WindowManager::mouse_callback(GLFWwindow *window, int button, int action, int mods)
{
    WindowManager *instance = &WindowManager::GetInstance();
    if (instance && instance->callbacks)
    {
        instance->callbacks->mouseCallback(window, button, action, mods);
    }
}

void WindowManager::resize_callback(GLFWwindow *window, int in_width, int in_height)
{
    WindowManager *instance = &WindowManager::GetInstance();
    if (instance && instance->callbacks)
    {
        instance->callbacks->resizeCallback(window, in_width, in_height);
    }
}
#include <GameManager.h>
#include <iostream>
#include <chrono>
#include <GLFW/glfw3.h>
#include <globals.h>
#include <glad/glad.h>

void process()
{
    // do game logic
}

void error_callback(int error, const char *description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

int GameManager::Initialize()
{
    std::cout << "Init" << std::endl;
    glfwSetErrorCallback(error_callback);
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a GLFW window
    window = glfwCreateWindow(800, 600, "My Game", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glClear(GL_COLOR_BUFFER_BIT);

    // Configure OpenGL
    // TODO: deep dive into this config
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 800.0, 0.0, 600.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    return 0;
}

void GameManager::Run()
{
    std::chrono::high_resolution_clock::time_point currentTime, previousTime;
    double frameTime, loopTime, delta;

    std::cout << "Triggering main loop..." << std::endl;
    frameTime = 1.0 / TARGET_FPS;
    currentTime = previousTime = std::chrono::high_resolution_clock::now();
    loopTime = 0.0;

    std::cout << "Starting main loop" << std::endl;
    while (!glfwWindowShouldClose(window))
    {
        currentTime = std::chrono::high_resolution_clock::now();
        delta = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - previousTime).count();
        previousTime = currentTime;

        loopTime += delta;

        while (loopTime >= frameTime)
        {
            process();
            loopTime -= frameTime;
        }

        // render
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // clean up
    glfwTerminate();
}

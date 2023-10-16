#include <GameManager.h>
#include <GameObject.h>
#include <Renderer.h>
#include <globals.h>

#include <iostream>
#include <chrono>

void process()
{
    // do game logic
}

bool GameManager::Initialize()
{
    windowManager = &WindowManager::GetInstance();
    if (!windowManager->Initialize(800, 600))
    {
        std::cerr << "Failed to Initialize Window Manager" << std::endl;
        return false;
    }

    // other init goes here

    std::cout << "Initialized GameManager" << std::endl;
    return true;
}

// TODO: handler class
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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

    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f, 0.5f, 0.0f};

    GameObject triangle = GameObject(0, 0, 0, 0, vertices, 3, 3, (char *)"../res/shaders/Basic.shader");
    while (!glfwWindowShouldClose(windowManager->window))
    {
        // screen color
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // ensure window scaling is up to date
        int width, height;
        glfwGetFramebufferSize(windowManager->window, &width, &height);
        glViewport(0, 0, width, height);

        currentTime = std::chrono::high_resolution_clock::now();
        delta = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - previousTime).count();
        previousTime = currentTime;

        loopTime += delta;

        while (loopTime >= frameTime)
        {
            process();
            loopTime -= frameTime;
        }

        processInput(windowManager->window);

        triangle.Render();

        glfwSwapBuffers(windowManager->window);
        glfwPollEvents();
    }

    // clean up
    glfwTerminate();
}

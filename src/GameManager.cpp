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
    {
        std::cout << "User Closed with ESC" << std::endl;
        glfwSetWindowShouldClose(window, true);
    }
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
        0.5f, 0.5f, 0.0f,   // top right 0
        0.5f, -0.5f, 0.0f,  // bottom right 1
        -0.5f, -0.5f, 0.0f, // bottom left 2
        -0.5f, 0.5f, 0.0f   // top left 3
    };
    unsigned int indices[] = {
        // note that we start from 0!
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    GameObject rectangle = GameObject(0, 0, 0, 0, vertices, 4, 3, indices, 6, (char *)"../res/shaders/Basic.shader");
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

        rectangle.Render();

        glfwSwapBuffers(windowManager->window);
        glfwPollEvents();
    }

    // clean up
    glfwTerminate();
}

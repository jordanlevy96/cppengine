#include <GameManager.h>
#include <GameObject3D.h>
#include <Shader.h>
#include <globals.h>

#include <iostream>
#include <chrono>

void process()
{
    // do game logic
}

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        std::cout << "User Closed with ESC" << std::endl;
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void mouseCallback(GLFWwindow *window, int button, int action, int mods)
{
    // Your mouse callback implementation
}

void resizeCallback(GLFWwindow *window, int in_width, int in_height)
{
    // Your resize callback implementation
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
    stbi_set_flip_vertically_on_load(true);

    cam = new Camera(glm::vec3(0.0f, 0.0f, -3.0f));
    cam->SetPerspective(45.0f, 800.0f, 600.0f);

    EventCallbacks *callbacks = new EventCallbacks(
        [](GLFWwindow *window, int key, int scancode, int action, int mods)
        {
            keyCallback(window, key, scancode, action, mods);
        },
        [](GLFWwindow *window, int button, int action, int mods)
        {
            mouseCallback(window, button, action, mods);
        },
        [](GLFWwindow *window, int in_width, int in_height)
        {
            resizeCallback(window, in_width, in_height);
        });

    windowManager->setEventCallbacks(callbacks);

    std::cout << "Initialized GameManager" << std::endl;
    return true;
}

void GameManager::Run()
{
    std::cout << "Starting main loop" << std::endl;

    std::chrono::high_resolution_clock::time_point currentTime, previousTime;
    double frameTime, loopTime, delta;

    frameTime = 1.0 / TARGET_FPS;
    currentTime = previousTime = std::chrono::high_resolution_clock::now();
    loopTime = 0.0;

    float vertices[] = {
        // positions          // colors           // texture coords
        0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // top right
        0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
        -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f   // top left
    };

    int vertexCount = sizeof(vertices) / sizeof(float);

    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    int indexCount = sizeof(indices) / sizeof(unsigned int);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    GameObject3D *rectangle = new GameObject3D(0, 0, 0, 0, vertices, vertexCount, indices, indexCount, (char *)"../res/shaders/Basic.shader");
    objects.push_back(rectangle);
    rectangle->Rotate(-55.0f, EulerAngles::ROLL);
    rectangle->AddTexture("../res/textures/container.jpg", false);
    rectangle->AddTexture("../res/textures/awesomeface.png", true);
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

        // rectangle.Transform((float)glfwGetTime(), glm::vec3(1.5f), glm::vec3(0.5f, -0.5f, 0.0f));
        cam->RenderAll(objects);

        glfwSwapBuffers(windowManager->window);
        glfwPollEvents();
    }

    std::cout << "Exited main loop" << std::endl;
}

void GameManager::Shutdown()
{
    delete cam;
    for (GameObject *obj : objects)
    {
        delete obj;
    }
    objects.clear();
    windowManager->shutdown();
}

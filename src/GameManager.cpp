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
        return;
    }

    GameManager &gm = GameManager::GetInstance();
    if (key == GLFW_KEY_W)
    {
        gm.cam->Translate(glm::vec3(0.0f, 0.0f, 0.1f));
    }
    else if (key == GLFW_KEY_A)
    {
        gm.cam->Translate(glm::vec3(0.1f, 0.0f, 0.0f));
    }
    else if (key == GLFW_KEY_S)
    {
        gm.cam->Translate(glm::vec3(0.0f, 0.0f, -0.1f));
    }
    else if (key == GLFW_KEY_D)
    {
        gm.cam->Translate(glm::vec3(-0.1f, 0.0f, 0.0f));
    }
    else if (key == GLFW_KEY_Z)
    {
        gm.cam->Translate(glm::vec3(0.0f, -0.05f, 0.0f));
    }
    else if (key == GLFW_KEY_X)
    {
        gm.cam->Translate(glm::vec3(0.0f, 0.05f, 0.0f));
    }
}

void mouseCallback(GLFWwindow *window, int button, int action, int mods) {}

void resizeCallback(GLFWwindow *window, int in_width, int in_height) {}

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

    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CW);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

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

    /* --------- Object Declarations --------- */

    GameObject3D *bunny = new GameObject3D("../res/shaders/Basic.shader", "../res/models/xbunny.obj");
    objects.push_back(bunny);
    // bunny->Scale(glm::vec3(2.0f));

    // GameObject3D *cube = new GameObject3D("../res/shaders/Basic.shader", "../res/models/cube.obj");
    // objects.push_back(cube);
    // cube->Scale(glm::vec3(0.2f));
    // cube->Translate(glm::vec3(-4.0f, -1.0f, 0.0f));
    // cube->Rotate(-55.0f, EulerAngles::ROLL);
    // cube->AddTexture("../res/textures/container.jpg", false);
    // cube->AddTexture("../res/textures/awesomeface.png", true);

    GameObject3D *light = new GameObject3D("../res/shaders/Basic.shader", "../res/models/cube.obj");
    objects.push_back(light);
    light->Scale(glm::vec3(0.3f));
    light->Translate(glm::vec3(2.0f, 1.0f, 0.0f));
    while (!glfwWindowShouldClose(windowManager->window))
    {
        // background color
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

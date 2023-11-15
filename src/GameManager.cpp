#include <GameManager.h>
#include <GameObject3D.h>
#include <Shader.h>
#include <Script.h>
#include <globals.h>

#include <iostream>
#include <chrono>

void process()
{
    // do game logic
}

glm::vec3 GetPositionFromTransform(const glm::mat4 &modelMatrix)
{
    // Extract translation components from the model matrix
    glm::vec3 position;
    position.x = modelMatrix[3][0];
    position.y = modelMatrix[3][1];
    position.z = modelMatrix[3][2];

    return position;
}

static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        std::cout << "User Closed with ESC" << std::endl;
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }

    GameManager &gm = GameManager::GetInstance();
    Camera *cam = gm.cam;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cam->Move(CameraDirections::FORWARD, gm.delta);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cam->Move(CameraDirections::BACK, gm.delta);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cam->Move(CameraDirections::LEFT, gm.delta);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cam->Move(CameraDirections::RIGHT, gm.delta);

    // up and down
    if (key == GLFW_KEY_Z)
    {
        gm.cam->Translate(glm::vec3(0.0f, -0.05f, 0.0f));
    }
    if (key == GLFW_KEY_X)
    {
        gm.cam->Translate(glm::vec3(0.0f, 0.05f, 0.0f));
    }
}

static void clickCallback(GLFWwindow *window, int button, int action, int mods) {}

static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
    if (glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        GameManager &gm = GameManager::GetInstance();
        gm.cam->RotateByMouse(xpos, ypos);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_RELEASE)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

static void resizeCallback(GLFWwindow *window, int in_width, int in_height) {}

static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    GameManager &gm = GameManager::GetInstance();

    int width, height;
    glfwGetWindowSize(gm.windowManager->window, &width, &height);

    Camera *cam = gm.cam;
    float fov = cam->fov - (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;

    cam->SetPerspective(fov, width, height);
}

bool GameManager::Initialize()
{
    windowManager = &WindowManager::GetInstance();
    if (!windowManager->Initialize(800, 600))
    {
        std::cerr << "Failed to Initialize Window Manager" << std::endl;
        return false;
    }

    ui = &UI::GetInstance();
    ui->Initialize(windowManager->window);

    stbi_set_flip_vertically_on_load(true);

    glm::vec3 camStart = glm::vec3(0.0f, 0.0f, -10.0f);
    cam = new Camera(camStart);

    EventCallbacks *callbacks = new EventCallbacks(
        [](GLFWwindow *window, int key, int scancode, int action, int mods)
        {
            keyCallback(window, key, scancode, action, mods);
        },
        [](GLFWwindow *window, int button, int action, int mods)
        {
            clickCallback(window, button, action, mods);
        },
        [](GLFWwindow *window, double xpos, double ypos)
        {
            cursorPosCallback(window, xpos, ypos);
        },
        [](GLFWwindow *window, int in_width, int in_height)
        {
            resizeCallback(window, in_width, in_height);
        },
        [](GLFWwindow *window, double xoffset, double yoffset)
        {
            scrollCallback(window, xoffset, yoffset);
        });

    windowManager->setEventCallbacks(callbacks);

    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CW);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    Script().Run((char *)"../res/scripts/hello.lua");

    return true;
}

void GameManager::Run()
{
    std::cout << "Starting main loop" << std::endl;

    std::chrono::high_resolution_clock::time_point currentTime, previousTime;
    double frameTime, loopTime;

    frameTime = 1.0 / TARGET_FPS;
    currentTime = previousTime = std::chrono::high_resolution_clock::now();
    loopTime = 0.0;

    /* --------- Initial State --------- */
    GameObject3D *bunny = new GameObject3D("../res/shaders/Lighting.shader", "../res/models/xbunny.obj");
    bunny->Scale(glm::vec3(3.0f));
    bunny->AddUniform("objectColor", glm::vec3(1.0f, 0.5f, 0.31f), UniformTypeMap::vec3);
    bunny->AddUniform("lightColor", glm::vec3(1.0f), UniformTypeMap::vec3);
    objects.push_back(bunny);

    GameObject3D *cube = new GameObject3D("../res/shaders/Basic.shader", "../res/models/cube.obj");
    cube->Scale(glm::vec3(0.2f));
    cube->Translate(glm::vec3(-4.0f, 6.0f, 10.0f));
    cube->Rotate(-55.0f, EulerAngles::ROLL);
    // cube->AddTexture("../res/textures/container.jpg", false);
    // cube->AddTexture("../res/textures/awesomeface.png", true);
    objects.push_back(cube);

    bunny->AddUniform("lightPos", GetPositionFromTransform(cube->transform), UniformTypeMap::vec3);

    while (!glfwWindowShouldClose(windowManager->window))
    {
        /* ------------- Main Loop -------------
            1. Process Game Logic
            2. Render Pipeline
            Render order:
                1. Background
                2. GameObjects
                3. UI
        */

        // Process
        currentTime = std::chrono::high_resolution_clock::now();
        delta = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - previousTime).count();
        previousTime = currentTime;

        loopTime += delta;

        while (loopTime >= frameTime)
        {
            process();
            loopTime -= frameTime;
        }

        bunny->AddUniform("viewPos", cam->pos, UniformTypeMap::vec3);

        // ensure window scaling is up to date before running render pipeline
        int width, height;
        glfwGetFramebufferSize(windowManager->window, &width, &height);
        glViewport(0, 0, width, height);

        // 1. Background
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 2. Game Objects
        cam->RenderAll(objects);

        // 3. UI
        ui->RenderWindow();

        glfwPollEvents();
        glfwSwapBuffers(windowManager->window);
    }

    std::cout << "Exited main loop" << std::endl;
}

void GameManager::Shutdown()
{
    delete cam;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    for (GameObject *obj : objects)
    {
        delete obj;
    }
    objects.clear();
    windowManager->shutdown();
}

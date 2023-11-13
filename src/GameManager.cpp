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

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    if (!ImGui_ImplGlfw_InitForOpenGL(windowManager->window, true) ||
        !ImGui_ImplOpenGL3_Init("#version 150"))
    {
        std::cerr << "ImGui initialization failed!" << std::endl;
        return -1;
    }

    ImGui::StyleColorsDark();

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

    // cube->AddTexture("../res/textures/container.jpg", false);
    // cube->AddTexture("../res/textures/awesomeface.png", true);

    // bunny->Scale(glm::vec3(3.0f));
    // bunny->Translate(glm::vec3(0.0f, -0.25f, 0.0f));

    // GameObject3D *light = new GameObject3D("../res/shaders/Basic.shader", "../res/models/cube.obj");
    // objects.push_back(light);
    // light->Scale(glm::vec3(0.3f));
    // light->Translate(glm::vec3(2.0f, 1.0f, 0.0f));

    // cam->RotateByMouse(0, 0);

    /* --------- Initial State --------- */
    GameObject3D *bunny = new GameObject3D("../res/shaders/Basic.shader", "../res/models/xbunny.obj");
    bunny->Scale(glm::vec3(3.0f));
    objects.push_back(bunny);

    GameObject3D *cube = new GameObject3D("../res/shaders/Basic.shader", "../res/models/cube.obj");
    cube->Scale(glm::vec3(0.2f));
    cube->Translate(glm::vec3(-4.0f, -1.0f, 0.0f));
    cube->Rotate(-55.0f, EulerAngles::ROLL);
    objects.push_back(cube);

    ImGuiIO &io = ImGui::GetIO();
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");          // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float *)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwPollEvents();
        glfwSwapBuffers(windowManager->window);

        // ui->RenderWindow();
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

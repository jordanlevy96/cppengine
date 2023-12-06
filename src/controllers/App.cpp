#include "controllers/App.h"
#include "Tetris.h"

#include "util/TransformUtils.h"

#include <iostream>
#include <chrono>

bool App::Initialize()
{
    const char *defaultSettingsPath = "../res/conf/settings.yaml";
    const char *userSettingsPath = std::getenv("USER_SETTINGS_PATH");

    std::string settingsPath = userSettingsPath ? userSettingsPath : defaultSettingsPath;
    LoadConfig(settingsPath);

    windowManager = &WindowManager::GetInstance();
    if (!windowManager->Initialize(conf.WindowWidth, conf.WindowHeight))
    {
        std::cerr << "Failed to Initialize Window Manager" << std::endl;
        return false;
    }

    ui = &UI::GetInstance();
    ui->Initialize(windowManager->window);

    stbi_set_flip_vertically_on_load(true);

    cam = new Camera(conf.WindowWidth, conf.WindowHeight);

    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CW);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    registry = &Registry::GetInstance();

    // Lua must be initialized last, as it needs references to the other controllers
    lua = &ScriptManager::GetInstance();
    lua->Initialize();
    lua->CreateTable(EVENT_QUEUE);

    Tetris::LoadTetriminos(conf.ResourcePath + "conf/tetriminos.yaml");

    return true;
}

void App::Run()
{
    std::cout << "Starting main loop" << std::endl;

    std::chrono::high_resolution_clock::time_point currentTime, previousTime;
    double frameTime, loopTime;

    frameTime = 1.0 / conf.targetFPS;
    currentTime = previousTime = std::chrono::high_resolution_clock::now();
    loopTime = 0.0;

    /* --------- Initial State --------- */
    registry->LoadScene(conf.ResourcePath + "scenes/MainScene.yaml");

    while (!glfwWindowShouldClose(windowManager->window))
    {
        /* ------------- Main Loop -------------
            1. Process Game Logic
            2. Input Handling
            3. Other Systems (Script)
            4. Render Pipeline
            Render order:
                1. Background
                2. GameObjects (RenderSystem)
                3. UI
        */

        // Process
        currentTime = std::chrono::high_resolution_clock::now();
        delta = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - previousTime).count();
        previousTime = currentTime;

        loopTime += delta;

        while (loopTime >= frameTime)
        {
            // do game logic here
            loopTime -= frameTime;
        }

        lua->ProcessInput();
        ScriptSystem::Update(delta);

        // ensure window scaling is up to date before running render pipeline
        int width, height;
        glfwGetFramebufferSize(windowManager->window, &width, &height);
        glViewport(0, 0, width, height);

        // 1. Background
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 2. Game Objects
        RenderSystem::Update(cam, delta);

        // 3. UI
        ui->RenderWindow();

        glfwPollEvents();
        glfwSwapBuffers(windowManager->window);
    }

    std::cout << "Exited main loop" << std::endl;
}

bool App::LoadConfig(const std::string &configPath)
{
    YAML::Node config = YAML::LoadFile(configPath);

    if (!config)
    {
        std::cerr << "Failed to read config from " << configPath << std::endl;
        return false;
    }

    YAML::Node input = config["input"];

    if (input)
    {
        // YAML::Node keyMappings = input["keyMappings"];
        // YAML::Node script = input["scriptPath"];
        // std::vector<std::string> actions;

        // for (auto key : keyMappings)
        // {
        //     std::string action = key.first.as<std::string>();
        //     actions.push_back(action);
        // }

        // std::string scriptPath = input["scriptPath"].as<std::string>();
        // LoadLuaScript(scriptPath);
    }

    YAML::Node window = config["window"];

    if (window)
    {
        YAML::Node fps = window["targetFPS"];
        YAML::Node width = window["windowWidth"];
        YAML::Node height = window["windowHeight"];

        if (fps)
        {
            conf.targetFPS = fps.as<float>();
        }

        if (width)
        {
            conf.WindowWidth = width.as<float>();
        }

        if (height)
        {
            conf.WindowHeight = height.as<float>();
        }
    }

    return true;
}

void App::Shutdown()
{
    delete cam;

    lua->Shutdown();
    registry->Shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    windowManager->Shutdown();
}

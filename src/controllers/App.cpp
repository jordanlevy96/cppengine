#include "controllers/App.h"

#include "systems/RenderSystem.h"
#include "systems/ScriptSystem.h"
#include "systems/TweenSystem.h"

#include "util/TransformUtils.h"

#include <iostream>
#include <fstream>
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
        std::cerr << "INIT - Window Manager: FAIL" << std::endl;
        return false;
    }

    ui = &UI::GetInstance();
    ui->Initialize(windowManager->window);

    htmlRenderer = &HTMLRendererMP::GetInstance();
    htmlRenderer->Initialize(windowManager->window, conf.WindowWidth, conf.WindowHeight);

    stbi_set_flip_vertically_on_load(true);

    cam = new Camera(conf.WindowWidth, conf.WindowHeight);

    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CW);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    registry = &Registry::GetInstance();

    // Scripting must be initialized last before the scene
    // as it needs references to the other controllers
    scriptManager = &ScriptManager::GetInstance();
    scriptManager->Initialize();

    // Tetris::LoadTetriminos(conf.ResourcePath + "conf/tetriminos.yaml");

    // Scene is loaded last
    registry->LoadScene("scenes/MainScene.yaml");

    // Load FPS test HTML UI
    std::string htmlPath = conf.ResourcePath + "ui/fps_test.html";
    std::ifstream htmlFile(htmlPath);
    if (htmlFile.is_open())
    {
        std::string htmlContent((std::istreambuf_iterator<char>(htmlFile)),
                                std::istreambuf_iterator<char>());
        htmlRenderer->LoadHTML(htmlContent);
        htmlFile.close();
        std::cout << "Loaded HTML UI from: " << htmlPath << std::endl;
    }
    else
    {
        std::cerr << "Failed to load HTML file: " << htmlPath << std::endl;
    }

    return true;
}

void App::Run()
{

    std::chrono::high_resolution_clock::time_point currentTime, previousTime;
    double frameTime, loopTime;

    frameTime = 1.0 / conf.targetFPS;
    currentTime = previousTime = std::chrono::high_resolution_clock::now();
    loopTime = 0.0;

    // FPS tracking
    int frameCount = 0;
    double fpsTime = 0.0;
    auto fpsUpdateTime = std::chrono::high_resolution_clock::now();

    std::cout << "Starting main loop" << std::endl;
    while (!glfwWindowShouldClose(windowManager->window))
    {
        /* ------------- Main Loop -------------
            1. Process Game Logic
            2. Input Handling
            3. Other Systems (Script, Tween)
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

        scriptManager->ProcessInput();

        while (loopTime >= frameTime)
        {
            loopTime -= frameTime;
            // do game logic here
            ScriptSystem::Update(delta);
        }

        TweenSystem::Update(delta);

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
        // TODO: implement UISystem
        // ui->RenderWindow();
        htmlRenderer->Render();

        glfwSwapBuffers(windowManager->window);
        glfwPollEvents();

        // FPS tracking
        frameCount++;
        auto now = std::chrono::high_resolution_clock::now();
        fpsTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - fpsUpdateTime).count();
        if (fpsTime >= 1000.0) // Update every second
        {
            double fps = frameCount / (fpsTime / 1000.0);
            double frameTime = fpsTime / frameCount;
            std::cout << "FPS: " << fps << " (frame time: " << frameTime << "ms)" << std::endl;

            // Update HTML UI with current FPS
            std::string updatedHTML =
                "<!DOCTYPE html><html><head><style>"
                "body { margin: 0; padding: 0; background: transparent; color: #00ff00; font-family: monospace; font-size: 16px; }"
                ".fps-display { position: absolute; top: 10px; right: 10px; padding: 15px; background: rgba(0,0,0,0.9); border: 3px solid #00ff00; min-width: 150px; }"
                ".fps-value { font-size: 32px; font-weight: bold; color: #00ff00; margin: 5px 0; }"
                ".label { color: #00ff00; font-size: 14px; margin-top: 10px; }"
                "</style></head><body>"
                "<div class='fps-display'>"
                "<div class='label'>FPS</div>"
                "<div class='fps-value'>" + std::to_string((int)fps) + "</div>"
                "<div class='label'>FrameTime</div>"
                "<div class='fps-value'>" + std::to_string(frameTime).substr(0, 5) + "ms</div>"
                "</div></body></html>";

            htmlRenderer->UpdateHTML(updatedHTML);

            frameCount = 0;
            fpsUpdateTime = now;
        }
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

    registry->Shutdown();

    htmlRenderer->Shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    windowManager->Shutdown();
    scriptManager->Shutdown();
}

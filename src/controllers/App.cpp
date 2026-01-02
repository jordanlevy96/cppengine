#include "controllers/App.h"

#include "systems/RenderSystem.h"
#include "systems/ScriptSystem.h"
#include "systems/TweenSystem.h"
#include "systems/ReactiveUI.h"
#include "systems/LuaUIState.h"

#include "util/TransformUtils.h"
#include "util/Logger.h"

#include <fstream>
#include <chrono>
#include <thread>
#include <cmath>

bool App::Initialize()
{
    const char *defaultSettingsPath = "../res/conf/settings.yaml";
    const char *userSettingsPath = std::getenv("USER_SETTINGS_PATH");

    std::string settingsPath = userSettingsPath ? userSettingsPath : defaultSettingsPath;
    LoadConfig(settingsPath);

    // Initialize logging system first
    imhotep::Logger::GetInstance().Initialize("logs/imhotep.log");
    LOG_INFO("=== Imhotep starting ===");
    LOG_INFO("Loaded config from {}", settingsPath);

    windowManager = &WindowManager::GetInstance();
    if (!windowManager->Initialize(conf.WindowWidth, conf.WindowHeight))
    {
        LOG_ERROR("Window Manager initialization failed");
        return false;
    }
    LOG_INFO("Window Manager initialized: {}x{}", conf.WindowWidth, conf.WindowHeight);

    // ui = &UI::GetInstance();
    // ui->Initialize(windowManager->window);

    htmlRenderer = &HTMLRendererMT::GetInstance();
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

    // Initialize Lua-based reactive UI system
    ReactiveUI &reactiveUI = ReactiveUI::GetInstance();

    // Create Lua UI state and load state file
    auto luaState = std::make_shared<LuaUIState>();
    if (!luaState->LoadStateFile("../res/ui/state/fps.lua"))
    {
        LOG_ERROR("Failed to load UI state file");
        return false;
    }

    // Bind Lua state to ReactiveUI
    reactiveUI.BindLuaState(luaState);

    // Load UI template from files
    // TODO: set up file references in scene
    std::string uiTemplate = ReactiveUI::LoadTemplateFromFiles(
        "../res/ui/templates/tetris.html",
        "../res/ui/styles/tetris.css");

    if (uiTemplate.empty())
    {
        LOG_ERROR("Failed to load UI template files");
        return false;
    }

    reactiveUI.RegisterTemplateWithDirectives("ui_template", uiTemplate);

    // CRITICAL: Set event handlers BEFORE loading HTML to avoid race condition
    // The render thread needs handlers available when it calls ExtractInteractiveElements()
    htmlRenderer->SetEventHandlers(reactiveUI.GetEventHandlers());
    LOG_INFO("Set {} event handlers before HTML render",
             reactiveUI.GetEventHandlers().size());

    // Now load HTML - render thread will extract elements with correct handlers
    htmlRenderer->LoadHTML(reactiveUI.GetRenderedHTML());

    // Initialize FPS tracking
    m_fpsUpdateTime = std::chrono::high_resolution_clock::now();

    return true;
}

float App::GetSimulationMultiplier() const
{
    switch (m_simSpeed)
    {
    case SimulationSpeed::PAUSED:
        return 0.0f;
    case SimulationSpeed::SLOW:
        return 0.5f;
    case SimulationSpeed::NORMAL:
        return 1.0f;
    case SimulationSpeed::FAST:
        return 2.0f;
    case SimulationSpeed::FASTER:
        return 3.0f;
    case SimulationSpeed::FASTEST:
        return 5.0f;
    case SimulationSpeed::UNCAPPED:
        return -1.0f; // Special: run as fast as possible
    default:
        return 1.0f;
    }
}

void App::Run()
{
    LOG_INFO("Starting main loop in {} mode", m_gameMode == GameMode::FIXED ? "FIXED" : "VARIABLE");

    if (m_gameMode == GameMode::VARIABLE)
    {
        RunVariableLoop();
    }
    else
    {
        RunFixedLoop();
    }
}

void App::RunFixedLoop()
{
    double frameTime = 1000.0 / conf.targetFPS; // 16.67ms for 60 FPS
    double accumulator = 0.0;

    // Enable VSync for fixed-speed games (prevents tearing)
    glfwSwapInterval(1);

    auto prevTime = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(windowManager->window))
    {
        auto currTime = std::chrono::high_resolution_clock::now();
        delta = std::chrono::duration_cast<std::chrono::milliseconds>(
                    currTime - prevTime)
                    .count();
        prevTime = currTime;

        accumulator += delta;

        scriptManager->ProcessInput();

        while (accumulator >= frameTime)
        {
            accumulator -= frameTime;
            ScriptSystem::Update(frameTime); // Pass fixed delta
        }

        TweenSystem::Update(delta);

        // Render every frame (coupled to game logic)
        Render();

        glfwSwapBuffers(windowManager->window); // VSync blocks here
        glfwPollEvents();

        TrackFPS();
    }

    LOG_INFO("Exited main loop");
}

void App::RunVariableLoop()
{
    double simFrameTime = 1000.0 / 60.0; // 16.67ms base tick
    double renderFrameTime = 1000.0 / conf.targetFPS;
    double simAccumulator = 0.0;
    double renderAccumulator = 0.0;

    // Disable VSync (we control frame timing)
    glfwSwapInterval(0);

    auto prevTime = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(windowManager->window))
    {
        auto currTime = std::chrono::high_resolution_clock::now();
        delta = std::chrono::duration_cast<std::chrono::milliseconds>(
                    currTime - prevTime)
                    .count();
        prevTime = currTime;

        scriptManager->ProcessInput();

        // Variable speed simulation
        float speedMultiplier = GetSimulationMultiplier();

        if (speedMultiplier == -1.0f)
        {
            // UNCAPPED: Run as many sim updates as possible
            int maxUpdates = 100; // Safety cap to prevent freeze
            for (int i = 0; i < maxUpdates; i++)
            {
                ScriptSystem::Update(simFrameTime);
            }
        }
        else if (speedMultiplier > 0.0f)
        {
            // NORMAL/FAST/etc: Run sim at multiplied speed
            simAccumulator += delta * speedMultiplier;

            while (simAccumulator >= simFrameTime)
            {
                simAccumulator -= simFrameTime;
                ScriptSystem::Update(simFrameTime);
            }
        }
        // If speedMultiplier == 0 (PAUSED), skip simulation entirely

        TweenSystem::Update(delta);

        // Render at capped framerate (decoupled from simulation)
        renderAccumulator += delta;

        if (renderAccumulator >= renderFrameTime)
        {
            renderAccumulator -= renderFrameTime;

            Render();

            glfwSwapBuffers(windowManager->window);
            TrackFPS();
        }

        glfwPollEvents();

        // Sleep to avoid spinning CPU
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           frameEnd - currTime)
                           .count();
        if (elapsed < 1.0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    LOG_INFO("Exited main loop");
}

void App::Render()
{
    // Ensure window scaling is up to date
    int width, height;
    glfwGetFramebufferSize(windowManager->window, &width, &height);
    glViewport(0, 0, width, height);

    // 1. Background
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. Game Objects
    RenderSystem::Update(cam, delta);

    // 3. UI
    htmlRenderer->Render();
}

// TODO: move as much as possible into Lua -- game-specific logic should not exist in C++
void App::TrackFPS()
{
    m_frameCount++;
    auto now = std::chrono::high_resolution_clock::now();
    m_fpsTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - m_fpsUpdateTime)
                    .count();

    // Update immediately after first 10 frames (to replace initial "0" values quickly)
    // Then update every second thereafter
    if ((m_frameCount == 10 && m_fpsTime < 1000.0) || m_fpsTime >= 1000.0)
    {
        int currentFPS = (int)(m_frameCount / (m_fpsTime / 1000.0));
        double currentFrameTime = m_fpsTime / m_frameCount;

        // LOG_DEBUG("FPS: {} (frame time: {}ms)", currentFPS, currentFrameTime);

        // Get reactive UI and Lua state
        ReactiveUI &reactiveUI = ReactiveUI::GetInstance();
        auto luaState = reactiveUI.GetLuaState();

        if (luaState)
        {
            // Update Lua state values - LuaUIState handles dirty flag
            luaState->SetValue("data.fps", currentFPS);
            luaState->SetValue("data.frameTime", std::to_string(currentFrameTime).substr(0, 5));

            // Update game mode and speed info
            const char *modeName = (m_gameMode == GameMode::FIXED) ? "FIXED" : "VARIABLE";
            luaState->SetValue("data.gameMode", std::string(modeName));

            if (m_gameMode == GameMode::VARIABLE)
            {
                // Show speed info for variable mode
                const char *speedName;
                switch (m_simSpeed)
                {
                case SimulationSpeed::PAUSED:
                    speedName = "PAUSED";
                    break;
                case SimulationSpeed::SLOW:
                    speedName = "SLOW";
                    break;
                case SimulationSpeed::NORMAL:
                    speedName = "NORMAL";
                    break;
                case SimulationSpeed::FAST:
                    speedName = "FAST";
                    break;
                case SimulationSpeed::FASTER:
                    speedName = "FASTER";
                    break;
                case SimulationSpeed::FASTEST:
                    speedName = "FASTEST";
                    break;
                case SimulationSpeed::UNCAPPED:
                    speedName = "UNCAPPED";
                    break;
                default:
                    speedName = "UNKNOWN";
                    break;
                }

                float multiplier = GetSimulationMultiplier();
                std::string multiplierStr = (multiplier == -1.0f) ? "MAX" : std::to_string(multiplier).substr(0, 3) + "x";

                luaState->SetValue("data.simSpeed", std::string(speedName));
                luaState->SetValue("data.simMultiplier", multiplierStr);
            }

            // GetRenderedHTML only re-renders if Lua state is dirty
            htmlRenderer->UpdateHTML(reactiveUI.GetRenderedHTML());
        }

        m_frameCount = 0;
        m_fpsUpdateTime = now;
    }
}

void App::StartGame()
{
    LOG_INFO("Game started");

    // Get reactive UI and Lua state
    ReactiveUI &reactiveUI = ReactiveUI::GetInstance();
    auto luaState = reactiveUI.GetLuaState();

    if (luaState)
    {
        // Update game state flags in UI (use boolean values, not strings!)
        luaState->SetValue("data.gameStarted", true);
        luaState->SetValue("data.gameOver", false);

        // Initialize game stats (will be updated by Lua when first piece spawns)
        luaState->SetValue("data.score", 0);
        luaState->SetValue("data.lines", 0);
        luaState->SetValue("data.level", 1);

        // Force re-render of UI to hide startup screen
        htmlRenderer->UpdateHTML(reactiveUI.GetRenderedHTML());
    }
}

void App::ResetGame()
{
    LOG_INFO("Restarting game");

    // Get reactive UI and Lua state
    ReactiveUI &reactiveUI = ReactiveUI::GetInstance();
    auto luaState = reactiveUI.GetLuaState();

    if (luaState)
    {
        // Reset UI state for restart (keep game started)
        luaState->SetValue("data.gameOver", false);
        luaState->SetValue("data.gameStarted", true);
        luaState->SetValue("data.score", 0);
        luaState->SetValue("data.lines", 0);
        luaState->SetValue("data.level", 1);

        // Force re-render of UI
        htmlRenderer->UpdateHTML(reactiveUI.GetRenderedHTML());
    }
}

void App::ReturnToMainMenu()
{
    LOG_INFO("Returning to main menu");

    // Get reactive UI and Lua state
    ReactiveUI &reactiveUI = ReactiveUI::GetInstance();
    auto luaState = reactiveUI.GetLuaState();

    if (luaState)
    {
        // Reset UI state back to startup screen
        luaState->SetValue("data.gameOver", false);
        luaState->SetValue("data.gameStarted", false);
        luaState->SetValue("data.score", 0);
        luaState->SetValue("data.lines", 0);
        luaState->SetValue("data.level", 1);

        // Force re-render of UI
        htmlRenderer->UpdateHTML(reactiveUI.GetRenderedHTML());
    }
}

bool App::LoadConfig(const std::string &configPath)
{
    YAML::Node config = YAML::LoadFile(configPath);

    if (!config)
    {
        LOG_ERROR("Failed to read config from {}", configPath);
        return false;
    }

    YAML::Node input = config["input"];

    // FIXME: ???
    // if (input)
    // {
    //     YAML::Node keyMappings = input["keyMappings"];
    // }

    YAML::Node window = config["window"];

    if (window)
    {
        conf.WindowWidth = window["windowWidth"].as<float>();
        conf.WindowHeight = window["windowHeight"].as<float>();
        conf.targetFPS = window["targetFPS"].as<float>();
    }

    // Resource path might not be in settings file
    if (config["resourcePath"])
    {
        conf.ResourcePath = config["resourcePath"].as<std::string>();
    }

    return true;
}

void App::CloseWindow()
{
    glfwSetWindowShouldClose(windowManager->window, true);
}

void App::Shutdown()
{
    LOG_INFO("Shutting down");
    htmlRenderer->Shutdown();
    windowManager->Shutdown();
    scriptManager->Shutdown();
    delete cam;
}

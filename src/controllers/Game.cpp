/**
 * @file Game.cpp
 * @brief Main game implementation using EngineCore for common initialization
 */

#include "controllers/Game.h"
#include "controllers/EngineCore.h"
#include "systems/RenderSystem.h"
#include "systems/ScriptSystem.h"
#include "systems/TweenSystem.h"
#include "util/TransformUtils.h"
#include "util/Logger.h"
#include "util/ConfigLoader.h"
#include <fstream>
#include <chrono>
#include <thread>
#include <cmath>

bool Game::Initialize()
{
    std::cout << "[Game] Starting initialization..." << std::endl;

    // Load config and initialize EngineCore
    if (!m_core.Initialize("../res/conf/settings.yaml", conf, &cam))
    {
        return false;
    }

    // Get references to subsystems
    windowManager = m_core.GetWindowManager();
    htmlRenderer = m_core.GetHTMLRenderer();
    registry = m_core.GetRegistry();
    scriptManager = m_core.GetScriptManager();

    // Initialize FPS tracking
    m_fpsUpdateTime = std::chrono::high_resolution_clock::now();

    LOG_INFO("Game initialization complete");
    return true;
}

float Game::GetSimulationMultiplier() const
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

void Game::Run()
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

void Game::RunFixedLoop()
{
    double frameTime = 1000.0 / conf.targetFPS; // 16.67ms for 60 FPS
    double accumulator = 0.0;

    // Enable VSync for fixed-speed games (prevents tearing)
    glfwSwapInterval(1);

    auto prevTime = std::chrono::high_resolution_clock::now();

    while (!m_core.ShouldClose())
    {
        // CRITICAL: Poll events first so window appears and responds
        glfwPollEvents();

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

        m_core.EndFrame();

        TrackFPS();
    }

    LOG_INFO("Exited main loop");
}

void Game::RunVariableLoop()
{
    double simFrameTime = 1000.0 / 60.0; // 16.67ms base tick
    double renderFrameTime = 1000.0 / conf.targetFPS;
    double simAccumulator = 0.0;
    double renderAccumulator = 0.0;

    // Disable VSync (we control frame timing)
    glfwSwapInterval(0);

    auto prevTime = std::chrono::high_resolution_clock::now();

    while (!m_core.ShouldClose())
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

            m_core.EndFrame();
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

void Game::Render()
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

// TODO: move into Lua -- game-specific logic should not (have to) exist in C++
void Game::TrackFPS()
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

void Game::StartGame()
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

void Game::ResetGame()
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

void Game::ReturnToMainMenu()
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

void Game::CloseWindow()
{
    glfwSetWindowShouldClose(windowManager->window, true);
}

void Game::Shutdown()
{
    LOG_INFO("Shutting down");
    htmlRenderer->Shutdown();
    windowManager->Shutdown();
    scriptManager->Shutdown();
    // Camera is now owned by EngineCore, no need to delete
}

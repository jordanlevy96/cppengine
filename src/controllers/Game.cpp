/**
 * @file Game.cpp
 * @brief Main game loop implementation (Tetris) using EngineCore
 * @lines ~285
 *
 * Purpose: Implements core game loop with fixed/variable timestep options.
 * Coordinates rendering, input processing, and Lua script updates.
 *
 * Key functions:
 * - Initialize() - Setup window, renderer, scene, UI (line 21, ~50 lines)
 * - Run() - Main loop dispatch to fixed or variable (line 70, ~15 lines)
 * - RunFixedLoop() - Fixed 60 FPS game loop (line 84, ~40 lines)
 * - RunVariableLoop() - Variable timestep game loop (line 126, ~50 lines)
 * - Render() - Coordinate 3D + UI rendering (line 179, ~15 lines)
 * - TrackFPS() - FPS counter and display (line 197, ~75 lines)
 * - Shutdown() - Cleanup resources (line 279, ~10 lines)
 *
 * Game loop modes:
 * - FIXED: 60 FPS locked, consistent physics timestep
 * - VARIABLE: Uncapped FPS, delta time per frame
 * - Mode selected via Lua state: data.gameMode
 *
 * Integration: Uses EngineCore for shared initialization, coordinates all systems
 */

#include "controllers/Game.h"
#include "controllers/EngineCore.h"
#include "systems/HierarchySystem.h"
#include "systems/RenderSystem.h"
#include "systems/ScriptSystem.h"
#include "systems/TweenSystem.h"
#include "systems/TiledBackgroundRenderer.h"
#include "util/TransformUtils.h"
#include "util/Logger.h"
#include "util/ConfigLoader.h"
#include "util/FrameTiming.h"
#include <chrono>
#include <thread>

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

    // Store Game instance in GLFW user pointer for resize callback
    glfwSetWindowUserPointer(windowManager->window, this);

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
    LOG_INFO("Starting FIXED loop (60 FPS)");

    // Initialize frame timing with fixed timestep
    FrameTiming timing(FrameTimingMode::FIXED, conf.targetFPS);
    timing.SetVSync(true); // Enable VSync for fixed-speed games
    glfwSwapInterval(1);

    while (!m_core.ShouldClose())
    {
        // CRITICAL: Poll events first so window appears and responds
        glfwPollEvents();

        // Update frame timing (calculates delta and accumulators)
        timing.Update();
        delta = timing.GetDelta();

        // Process input
        scriptManager->ProcessInput();

        // Update game logic at fixed timestep
        while (timing.ShouldUpdateFixedStep())
        {
            ScriptSystem::Update(timing.GetFixedDelta());
        }

        // Update animation and hierarchy
        TweenSystem::Update(delta);
        HierarchySystem::Update();

        // Render (every frame in fixed mode)
        Render();
        m_core.EndFrame();

        // Track FPS for UI display
        TrackFPS();
    }

    LOG_INFO("Exited FIXED main loop");
}

void Game::RunVariableLoop()
{
    LOG_INFO("Starting VARIABLE loop (decoupled sim/render)");

    // Initialize frame timing with variable speed (60 FPS sim, conf.targetFPS render)
    FrameTiming timing(FrameTimingMode::VARIABLE, 60.0, conf.targetFPS);
    timing.SetVSync(false); // We control frame timing
    glfwSwapInterval(0);

    while (!m_core.ShouldClose())
    {
        // CRITICAL: Poll events first so window appears and responds
        glfwPollEvents();

        // Update frame timing (calculates delta and accumulators)
        timing.Update();
        delta = timing.GetDelta();

        // Sync simulation speed with current setting
        timing.SetSimulationMultiplier(GetSimulationMultiplier());

        // Process input
        scriptManager->ProcessInput();

        // Update game logic at variable speed
        while (timing.ShouldUpdateSimulation())
        {
            ScriptSystem::Update(timing.GetFixedDelta());
        }

        // Update animation and hierarchy
        TweenSystem::Update(delta);
        HierarchySystem::Update();

        // Render at capped framerate (decoupled from simulation)
        if (timing.ShouldRenderFrame())
        {
            Render();
            m_core.EndFrame();
            TrackFPS();
        }

        // Sleep to avoid spinning CPU
        auto frameElapsed = timing.GetFrameElapsedMS();
        if (frameElapsed < 1.0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    LOG_INFO("Exited VARIABLE main loop");
}

void Game::Render()
{
    // Ensure window scaling is up to date
    int width, height;
    glfwGetFramebufferSize(windowManager->window, &width, &height);
    glViewport(0, 0, width, height);

    // 0. Clear to vaporwave sky color (background)
    glClearColor(0.2f, 0.7f, 0.9f, 1.0f);  // Cyan sky
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 0.5 Procedural tiled background (optional)
    TiledBackgroundRenderer::GetInstance().Render(cam, static_cast<float>(delta));

    // 1. Game Objects
    RenderSystem::Update(cam, delta);

    // 2. UI
    htmlRenderer->Render();
}

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

void Game::CloseWindow()
{
    glfwSetWindowShouldClose(windowManager->window, true);
}

void Game::Shutdown()
{
    LOG_INFO("[Game] Shutting down");

    htmlRenderer->Shutdown();
    windowManager->Shutdown();
    scriptManager->Shutdown();
    LOG_INFO("[Game] Shutdown complete");
}

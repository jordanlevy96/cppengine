/**
 * @file Game.h
 * @brief Main game singleton and game loop controller
 */

#pragma once

#include "Camera.h"
#include "controllers/WindowManager.h"
#include "controllers/Registry.h"
#include "controllers/EngineCore.h"
#include "systems/UI.h"
#include "systems/HTMLRendererMT.h"
#include "util/Config.h"
#include <memory>
#include <pybind11/pybind11.h>

/**
 * @brief Simulation speed multipliers for strategy games
 *
 * Controls how fast game logic updates relative to real time.
 * Only applies when GameMode is VARIABLE.
 */
enum class SimulationSpeed
{
    PAUSED = 0,  ///< Game paused (0x speed)
    SLOW = 1,    ///< Half speed (0.5x)
    NORMAL = 2,  ///< Real time (1x speed, default)
    FAST = 3,    ///< Double speed (2x)
    FASTER = 4,  ///< Triple speed (3x)
    FASTEST = 5, ///< 5x speed
    UNCAPPED = 6 ///< Max CPU speed (no frame limiter)
};

/**
 * @brief Game loop behavior mode
 *
 * Determines coupling between simulation and rendering.
 */
enum class GameMode
{
    FIXED,   ///< Fixed timestep, tight coupling (Tetris, platformers, action)
    VARIABLE ///< Variable speed, decoupled rendering (strategy, RTS, 4X)
};

/**
 * @brief Main game singleton managing game loop and core systems
 *
 * Coordinates initialization, main loop, rendering, and shutdown.
 * Supports both fixed-timestep (action games) and variable-speed (strategy games) modes.
 *
 * Uses EngineCore for common initialization to reduce duplication with Editor.
 *
 * **Initialization order:**
 * 1. LoadConfig() - Parse settings.yaml
 * 2. EngineCore initialization (Logger, Window, HTML Renderer)
 * 3. Camera, Registry (ECS), ScriptManager (Lua/Python)
 * 4. UI template loading
 *
 * **Game loop modes:**
 * - FIXED: Consistent 60 FPS update rate (default for Tetris)
 * - VARIABLE: Adjustable simulation speed (for strategy games)
 */
class Game
{
public:
    Config conf;                  ///< Application configuration
    Camera *cam;                  ///< 3D camera (nullptr if not using 3D)
    HTMLRendererMT *htmlRenderer; ///< Multi-threaded HTML renderer
    double delta = 0;             ///< Time since last frame (seconds)
    Registry *registry;           ///< Entity Component System registry
    WindowManager *windowManager; ///< Window and input manager
    ScriptManager *scriptManager; ///< Lua/Python script manager

    /**
     * @brief Get singleton instance
     * @return Reference to Game singleton
     */
    static Game &GetInstance()
    {
        static Game instance;
        return instance;
    }

    /**
     * @brief Get Python-wrapped singleton instance
     * @return Python object wrapping Game instance
     * @note Used for Python bindings, maintains reference semantics
     */
    static py::object GetPyInstance()
    {
        static py::object instance = py::cast(&GetInstance(), py::return_value_policy::reference);
        return instance;
    }

    Game(Game const &) = delete;
    void operator=(Game const &) = delete;

    /**
     * @brief Initialize all engine systems
     * @return true if initialization succeeded, false on failure
     * @note Loads config from settings.yaml, initializes logging, window, systems
     */
    bool Initialize();

    /**
     * @brief Shutdown all engine systems and cleanup resources
     */
    void Shutdown();

    /**
     * @brief Start main game loop (blocks until window closes)
     * @note Dispatches to RunFixedLoop() or RunVariableLoop() based on GameMode
     */
    void Run();

    /**
     * @brief Signal window to close (exits game loop)
     */
    void CloseWindow();

    /**
     * @brief Set simulation speed multiplier
     * @param speed Desired simulation speed
     * @note Only affects VARIABLE game mode
     */
    void SetSimulationSpeed(SimulationSpeed speed) { m_simSpeed = speed; }

    /**
     * @brief Get current simulation speed setting
     * @return Current SimulationSpeed enum value
     */
    SimulationSpeed GetSimulationSpeed() const { return m_simSpeed; }

    /**
     * @brief Get numeric simulation multiplier (0.0 = paused, 1.0 = normal)
     * @return Multiplier value based on current SimulationSpeed
     */
    float GetSimulationMultiplier() const;

    /**
     * @brief Set game loop behavior mode
     * @param mode FIXED (action) or VARIABLE (strategy)
     */
    void SetGameMode(GameMode mode) { m_gameMode = mode; }

    /**
     * @brief Get current game loop mode
     * @return Current GameMode enum value
     */
    GameMode GetGameMode() const { return m_gameMode; }

    /**
     * @brief Start new game session
     * @note Triggers game-specific initialization (called from UI/scripts)
     */
    void StartGame();

    /**
     * @brief Reset game to initial state
     * @note Reloads scene, resets entities
     */
    void ResetGame();

    /**
     * @brief Return to main menu screen
     * @note Cleans up game entities, shows menu UI
     */
    void ReturnToMainMenu();

private:
    Game() {};

    /**
     * @brief Fixed timestep game loop (60 FPS consistent updates)
     */
    void RunFixedLoop();

    /**
     * @brief Variable speed game loop (adjustable simulation rate)
     */
    void RunVariableLoop();

    /**
     * @brief Execute rendering for current frame
     */
    void Render();

    /**
     * @brief Update FPS counter for display
     */
    void TrackFPS();

    // Core engine helper
    EngineCore m_core;

    SimulationSpeed m_simSpeed = SimulationSpeed::NORMAL; ///< Current sim speed
    GameMode m_gameMode = GameMode::FIXED;                ///< Current loop mode

    // FPS tracking
    int m_frameCount = 0;                                           ///< Frames since last FPS update
    double m_fpsTime = 0.0;                                         ///< Accumulated time for FPS calc
    std::chrono::high_resolution_clock::time_point m_fpsUpdateTime; ///< Last FPS update timestamp
};

#pragma once

#include "Camera.h"
#include "controllers/WindowManager.h"
#include "controllers/Registry.h"
#include "systems/UI.h"
#include "systems/HTMLRendererMT.h"
#include <memory>
#include <pybind11/pybind11.h>

extern "C" void stbi_set_flip_vertically_on_load(int flag);

struct Config
{
    // default values can be overwritten via YAML
    float WindowWidth = 800;
    float WindowHeight = 600;
    float targetFPS = 60;
    std::string ResourcePath = "../res/";
};

// Simulation speed settings for strategy game
enum class SimulationSpeed {
    PAUSED = 0,      // 0x speed
    SLOW = 1,        // 0.5x speed
    NORMAL = 2,      // 1x speed (default)
    FAST = 3,        // 2x speed
    FASTER = 4,      // 3x speed
    FASTEST = 5,     // 5x speed
    UNCAPPED = 6     // As fast as CPU allows
};

// Game loop behavior mode
enum class GameMode {
    FIXED,       // Fixed simulation speed, tight coupling (Tetris, platformers, action games)
    VARIABLE     // Variable simulation speed, decoupled rendering (strategy games, RTS, city builders)
};

class App
{
public:
    Config conf;
    Camera *cam;
    UI *ui;
    HTMLRendererMT *htmlRenderer;
    // time since last frame
    double delta = 0;
    Registry *registry;
    WindowManager *windowManager;
    ScriptManager *scriptManager;

    static App &GetInstance()
    {
        static App instance;
        return instance;
    }

    static py::object GetPyInstance()
    {
        static py::object instance = py::cast(&GetInstance(), py::return_value_policy::reference);
        return instance;
    }

    App(App const &) = delete;
    void operator=(App const &) = delete;

    bool Initialize();
    void Shutdown();
    void Run();
    void CloseWindow();

    // Simulation speed controls
    void SetSimulationSpeed(SimulationSpeed speed) { m_simSpeed = speed; }
    SimulationSpeed GetSimulationSpeed() const { return m_simSpeed; }
    float GetSimulationMultiplier() const;

    // Game mode controls
    void SetGameMode(GameMode mode) { m_gameMode = mode; }
    GameMode GetGameMode() const { return m_gameMode; }

private:
    App() {};
    bool LoadConfig(const std::string &configPath);

    // Game loop implementations
    void RunFixedLoop();
    void RunVariableLoop();
    void Render();
    void TrackFPS();

    // Simulation speed and mode
    SimulationSpeed m_simSpeed = SimulationSpeed::NORMAL;
    GameMode m_gameMode = GameMode::FIXED;  // Default to fixed mode

    // FPS tracking state
    int m_frameCount = 0;
    double m_fpsTime = 0.0;
    std::chrono::high_resolution_clock::time_point m_fpsUpdateTime;
};
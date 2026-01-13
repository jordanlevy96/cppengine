/**
 * @file Editor.h
 * @brief Main editor application controller
 *
 * Phase 1: Basic window with HTML UI panels
 */

#pragma once

#include "controllers/EngineCore.h"
#include "controllers/WindowManager.h"
#include "controllers/Registry.h"
#include "systems/HTMLRendererMT.h"
#include "systems/ReactiveUI.h"
#include "editor/SceneViewport.h"
#include "util/Config.h"

#include <chrono>

/**
 * @brief Editor application singleton
 *
 * Manages editor lifecycle, UI state, and coordinates between
 * engine systems for editing workflows.
 *
 * **Phase 1 scope**: Window initialization, basic panel layout
 */
class Editor
{
public:
    /**
     * @brief Get singleton instance
     * @return Reference to Editor singleton
     */
    static Editor &GetInstance()
    {
        static Editor instance;
        return instance;
    }

    Editor(Editor const &) = delete;
    void operator=(Editor const &) = delete;

    /**
     * @brief Initialize editor window and UI
     * @return true if initialization succeeded
     */
    bool Initialize();

    /**
     * @brief Run editor main loop
     * @note Blocking call until editor is closed
     */
    void Run();

    /**
     * @brief Cleanup and shutdown editor
     */
    void Shutdown();

private:
    Editor() = default;
    ~Editor() { Shutdown(); }

    /**
     * @brief Render editor frame
     * @note Called every frame from Run()
     */
    void Render();

    /**
     * @brief Load editor UI templates
     */
    void LoadEditorUI();

    /**
     * @brief Update scene tree data in Lua state
     * @note Populates entity list with names and hierarchy
     */
    void UpdateSceneTree();

    /**
     * @brief Update game systems (scripts, tweens, etc.)
     * @param deltaTime Time since last update in milliseconds
     */
    void UpdateSystems(double deltaTime);

    // Configuration
    Config conf;

    // Core engine helper
    EngineCore m_core;

    // Singletons (references to engine systems)
    WindowManager *m_windowManager = nullptr;
    Registry *m_registry = nullptr;
    HTMLRendererMT *m_htmlRenderer = nullptr;
    ReactiveUI *m_reactiveUI = nullptr;
    Camera *m_camera = nullptr;

    // Editor-specific systems
    SceneViewport *m_viewport = nullptr;

    // Editor state
    bool m_initialized = false;
    bool m_shouldClose = false;

    // Timing for game loop
    double m_delta = 0.0;
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;

    // Input handler IDs
    size_t m_keyboardHandlerId = 0;
};

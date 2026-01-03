/**
 * @file Editor.h
 * @brief Main editor application controller
 * 
 * Phase 1: Basic window with HTML UI panels
 */

#pragma once

#include "controllers/WindowManager.h"
#include "controllers/Registry.h"
#include "systems/HTMLRendererMT.h"
#include "systems/ReactiveUI.h"

/**
 * @brief Editor application singleton
 * 
 * Manages editor lifecycle, UI state, and coordinates between
 * engine systems for editing workflows.
 * 
 * **Phase 1 scope**: Window initialization, basic panel layout
 */
class Editor {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to Editor singleton
     */
    static Editor& GetInstance() {
        static Editor instance;
        return instance;
    }

    Editor(Editor const&) = delete;
    void operator=(Editor const&) = delete;

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

    // Singletons (references to engine systems)
    WindowManager* m_windowManager = nullptr;
    Registry* m_registry = nullptr;
    HTMLRendererMT* m_htmlRenderer = nullptr;
    ReactiveUI* m_reactiveUI = nullptr;

    // Editor state
    bool m_initialized = false;
    bool m_shouldClose = false;

    // Input handler IDs
    size_t m_keyboardHandlerId = 0;
};

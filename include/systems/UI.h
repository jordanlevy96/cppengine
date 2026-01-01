/**
 * @file UI.h
 * @brief Dear ImGui debug UI singleton for development tools
 */

#pragma once

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

/**
 * @brief Singleton wrapper for Dear ImGui debug interface
 *
 * Provides immediate-mode GUI for debugging, development tools, and runtime tweaking.
 * Separate from HTMLRendererMT (which handles game UI).
 *
 * **Features:**
 * - Docking: Windows can be docked into layouts
 * - Multi-viewport: Windows can be dragged outside main window
 * - Keyboard/Gamepad navigation
 *
 * **Current Usage:**
 * - Demo window with FPS counter (example implementation)
 * - Intended for future debug panels, entity inspector, profiler
 *
 * **Integration:**
 * - Initialize(): Setup ImGui context and backends (called from App::Initialize)
 * - RenderWindow(): Draw UI widgets (called each frame from App::Render)
 * - Shutdown(): Cleanup ImGui resources (called from App::Shutdown)
 *
 * @note This is for DEVELOPMENT UI only, not production game UI
 * @note Game UI should use HTMLRendererMT with Lua-based templates
 * @see HTMLRendererMT for production game UI rendering
 */
class UI
{
public:
    /**
     * @brief Get singleton instance
     * @return Reference to UI singleton
     */
    static UI &GetInstance()
    {
        static UI instance;
        return instance;
    }

    UI(UI const &) = delete;
    void operator=(UI const &) = delete;

    /**
     * @brief Initialize ImGui context and backends
     * @param window GLFW window for rendering context
     * @note Configures docking, viewports, keyboard/gamepad navigation
     */
    void Initialize(GLFWwindow *window);

    /**
     * @brief Shutdown ImGui and cleanup resources
     * @note Must be called before destroying GLFW window
     */
    void Shutdown();

    /**
     * @brief Render ImGui windows for current frame
     * @note Call between OpenGL clear and swap buffers
     * @note Currently renders demo windows (FPS counter, widget examples)
     */
    void RenderWindow();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);  ///< Background clear color (demo window)
    bool show_another_window = false;                         ///< Toggle for demo second window

private:
    UI(){};
};

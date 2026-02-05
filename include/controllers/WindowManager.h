/**
 * @file WindowManager.h
 * @brief GLFW window and input event management
 */

#pragma once

#include "util/InputEvent.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>

/**
 * @brief GLFW window manager singleton handling window lifecycle and input
 *
 * Creates OpenGL context via GLFW, manages window state, and dispatches
 * input events to ScriptManager for Lua/Python handling.
 *
 * **Input Flow:**
 * GLFW callbacks → InputEvent → ScriptManager → Lua/Python handlers
 *
 * **Supported inputs:**
 * - Keyboard (key press/release)
 * - Mouse clicks
 * - Mouse movement
 * - Window resize
 * - Scroll wheel
 */
class WindowManager
{
public:
    /**
     * @brief Get singleton instance
     * @return Reference to WindowManager singleton
     */
    static WindowManager &GetInstance()
    {
        static WindowManager instance;
        return instance;
    }

    WindowManager(WindowManager const &) = delete;
    void operator=(WindowManager const &) = delete;

    /**
     * @brief Initialize GLFW window and OpenGL context
     * @param width Initial window width
     * @param height Initial window height
     * @return true if initialization succeeded
     * @note Creates OpenGL 3.3 Core Profile context
     */
    bool Initialize(int const width, int const height);

    /**
     * @brief Shutdown GLFW and destroy window
     */
    void Shutdown();

    /**
     * @brief Signal window to close (exits main loop)
     */
    void CloseWindow();

    /**
     * @brief Get current window dimensions
     * @return vec2 (width, height)
     */
    glm::vec2 GetSize();

    /**
     * @brief Register C++ input handler (called before Lua)
     * @param handler Callback that returns true to consume event
     * @return Handler ID for unregistration
     * @note Handlers are called in registration order. First handler to return true consumes event.
     */
    size_t RegisterInputHandler(InputHandler handler);

    /**
     * @brief Unregister input handler by ID
     * @param id Handler ID from RegisterInputHandler
     */
    void UnregisterInputHandler(size_t id);

    GLFWwindow *window; ///< GLFW window handle (public for renderer access)

private:
    WindowManager() {};

    /// GLFW keyboard callback (dispatches to ScriptManager)
    static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

    /// GLFW mouse button callback
    static void click_callback(GLFWwindow *window, int button, int action, int mods);

    /// GLFW cursor position callback
    static void cursorPos_callback(GLFWwindow *window, double xpos, double ypos);

    /// GLFW window resize callback
    static void resize_callback(GLFWwindow *window, int in_width, int in_height);

    /// GLFW scroll wheel callback
    static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

    /// GLFW character input callback (for text fields)
    static void char_callback(GLFWwindow *window, unsigned int codepoint);

    /// Input handler storage (handler ID, callback)
    std::vector<std::pair<size_t, InputHandler>> m_inputHandlers;

    /// Next handler ID for registration
    size_t m_nextHandlerId = 0;

    bool m_isShutdown = false;
};

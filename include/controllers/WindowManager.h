/**
 * @file WindowManager.h
 * @brief GLFW window and input event management
 */

#pragma once

#include "controllers/ScriptManager.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <functional>
#include <vector>
#include <variant>

/// Input event types for callback dispatching
enum InputTypes
{
    Key,    ///< Keyboard input
    Click,  ///< Mouse button click
    MouseButton, ///< Mouse button press/release (for mousedown/mouseup)
    Cursor, ///< Mouse cursor movement
    Resize, ///< Window resize event
    Scroll, ///< Mouse scroll wheel
    Char    ///< Character input (for text fields)
};

/**
 * @brief Input event data passed to scripts
 */
struct InputEvent
{
    InputTypes type;                                       ///< InputTypes enum value
    std::variant<std::string, glm::vec2, glm::vec3, glm::vec4> input; ///< Event data (key name, 2D position, click data, or mouse button data)
    ///< vec3 format for clicks: (x, y, button) where button: 0=left, 1=right, 2=middle
    ///< vec4 format for mouse buttons: (x, y, button, action) where action is GLFW_PRESS/GLFW_RELEASE
    int mods = 0; ///< Keyboard modifiers (GLFW_MOD_CONTROL, GLFW_MOD_SHIFT, etc.)
};

/**
 * @brief Input handler callback type
 * @param event The input event to handle
 * @return true to consume event (prevent propagation to Lua), false to pass through
 */
using InputHandler = std::function<bool(const InputEvent&)>;

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

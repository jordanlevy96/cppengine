/**
 * @file InputEvent.h
 * @brief Input event types and structures for the engine input system
 *
 * Standalone header to avoid coupling between WindowManager and ScriptManager.
 * Used by: WindowManager (dispatch), ScriptManager (Lua queue), EngineCore (UI handlers),
 *          Editor (editor input handling)
 */

#pragma once

#include <glm/glm.hpp>

#include <functional>
#include <string>
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
    int action = 0; ///< Action for key/click events (GLFW_PRESS, GLFW_RELEASE, GLFW_REPEAT)
};

/**
 * @brief Input handler callback type
 * @param event The input event to handle
 * @return true to consume event (prevent propagation to Lua), false to pass through
 */
using InputHandler = std::function<bool(const InputEvent&)>;

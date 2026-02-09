/**
 * @file InputEvent.h
 * @brief Input event types and structures for the engine input system
 *
 * Standalone header to avoid coupling between WindowManager and ScriptManager.
 * Used by: WindowManager (dispatch), ScriptManager (Lua queue), EngineCore (UI handlers),
 *          Editor (editor input handling)
 *
 * ## Input Event Architecture
 *
 * The engine uses a three-tiered input dispatch system with clear priority ordering:
 *
 * ### 1. C++ Input Handlers (Highest Priority - LIFO)
 *
 * Registered via `WindowManager::RegisterInputHandler(InputHandler)`.
 *
 * **Dispatch Order**: REVERSE registration order (Last In, First Out).
 * - Handlers registered later have higher priority
 * - Allows UI layers to intercept events before game logic
 * - Example: Editor input handlers can consume events before game input
 *
 * **Event Consumption**:
 * - Return `true` to consume event (stops propagation to Lua)
 * - Return `false` to pass through to next handler or Lua queue
 *
 * **Use Cases**:
 * - UI click detection (EngineCore registers handlers for HTMLRendererMT)
 * - Editor input interception (Editor registers keyboard/mouse handlers)
 * - Debug overlays and tools
 *
 * **Example**:
 * ```cpp
 * size_t handlerId = windowManager->RegisterInputHandler([](const InputEvent& e) {
 *     if (e.type == InputTypes::Click) {
 *         // Check if click is on UI element
 *         if (isClickOnUI(e)) {
 *             handleUIClick(e);
 *             return true; // Consume - don't send to Lua
 *         }
 *     }
 *     return false; // Pass through
 * });
 * ```
 *
 * ### 2. Lua Event Queue (Medium Priority - If Not Consumed)
 *
 * Events that C++ handlers don't consume are queued to Lua via
 * `ScriptManager::AddInputEventToQueue(event)`.
 *
 * **Processing**:
 * - Queued as native Lua tables to `EventQueue` global
 * - Processed by `HandleInput()` function in game scripts
 * - Asynchronous - processed on next Lua script update
 *
 * **Use Cases**:
 * - Game logic input (player movement, actions)
 * - Camera controls
 * - Game-specific key bindings
 *
 * **Example Lua Handler** (res/scripts/games/tetris/TetrisInput.lua):
 * ```lua
 * function HandleInput()
 *     for i, event in ipairs(EventQueue) do
 *         if event.type == "Key" then
 *             if event.input == "LEFT" then
 *                 grid:moveLeft()
 *             end
 *         end
 *     end
 * end
 * ```
 *
 * ### 3. UI Event Handlers (Specific to UI Elements)
 *
 * Triggered by C++ handlers checking if input coordinates intersect UI elements,
 * then dispatched via `ReactiveUI::DispatchEvent()`.
 *
 * **HTML Template Bindings**:
 * - `@click="methodName"` - Click events
 * - `@mousedown="methodName"` - Mouse press
 * - `@mouseup="methodName"` - Mouse release
 * - `@keydown="methodName"` - Key press on focused element
 *
 * **Execution**:
 * - C++ handler (registered by EngineCore) detects UI hit
 * - ReactiveUI parses event attribute from HTML
 * - Calls Lua method in UI state
 *
 * **Use Cases**:
 * - Button clicks in UI
 * - Text input fields
 * - UI-specific interactions
 *
 * **Example** (res/ui/templates/game.html):
 * ```html
 * <button @click="onStartGame">START GAME</button>
 * ```
 * ```lua
 * -- res/ui/state/game.lua
 * methods = {
 *     onStartGame = function(self)
 *         GameManager:start()
 *     end
 * }
 * ```
 *
 * ## Event Flow Diagram
 *
 * ```
 * GLFW Event (keyboard/mouse)
 *     ↓
 * WindowManager::*_callback()
 *     ↓
 * [1] Try C++ handlers (REVERSE order, LIFO)
 *     ├─→ Handler returns true → Event consumed (STOP)
 *     └─→ All return false → Continue to [2]
 *     ↓
 * [2] ScriptManager::AddInputEventToQueue()
 *     ├─→ UI handler detects hit → ReactiveUI::DispatchEvent() [3]
 *     └─→ Otherwise → Lua HandleInput() processes queue
 *     ↓
 * [3] ReactiveUI executes Lua method from template
 * ```
 *
 * ## Priority Summary
 *
 * **Highest → Lowest**:
 * 1. Last registered C++ handler
 * 2. Earlier registered C++ handlers (LIFO order)
 * 3. Lua event queue (game scripts)
 * 4. UI-specific handlers (triggered by C++ UI handlers)
 *
 * ## Important Notes
 *
 * - **Handler Registration Order**: Use LIFO priority wisely. Register critical
 *   intercept handlers (UI, editor) AFTER game handlers so they get first chance.
 * - **Event Types**: Not all event types go through all paths. `MouseButton` events
 *   are only sent to C++ handlers, not Lua queue (see WindowManager.cpp:357-375).
 * - **Consumption is Final**: Once a C++ handler consumes an event (returns true),
 *   Lua scripts will never see it. Be careful not to block game input accidentally.
 *
 * @see WindowManager::RegisterInputHandler() for C++ handler registration
 * @see ScriptManager::AddInputEventToQueue() for Lua queue
 * @see ReactiveUI::DispatchEvent() for UI event dispatch
 * @see docs/architecture/UI_SYSTEM.md for full UI architecture
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

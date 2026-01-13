/**
 * @file ScriptSystem.h
 * @brief ECS system for executing Lua and Python scripts attached to entities
 */

#pragma once

#include "controllers/ScriptManager.h"
#include "controllers/Registry.h"

/**
 * @brief Static system for updating entity-attached Lua and Python scripts
 *
 * Iterates through entities with ScriptComponent and invokes their update functions.
 * Supports both Lua (primary) and Python (experimental) scripting languages.
 *
 * **Lua Script Pattern:**
 * Scripts implement a table with `ready()` and `process(self, delta)` functions.
 * - `ready()`: Called once when ScriptComponent is created
 * - `process()`: Called every frame during Update()
 *
 * Example Lua script:
 * @code
 * local MyScript = {}
 * function MyScript:ready()
 *     print("Script initialized")
 * end
 * function MyScript:process(delta)
 *     -- Update logic here
 * end
 * return MyScript
 * @endcode
 *
 * **Python Script Pattern:**
 * Scripts implement a class with `update(self, delta)` method.
 *
 * **Error Handling:**
 * - Lua errors caught via sol::error and logged to console
 * - Python errors propagate as exceptions (less robust)
 *
 * @note All methods are static - this is a stateless system
 * @note Scripts execute on main thread during game loop
 * @see ScriptComponent for script attachment details
 */
class ScriptSystem
{
public:
    /**
     * @brief Execute update functions for all entity-attached scripts
     * @param delta Time since last frame in seconds
     * @note Calls Lua process(delta) or Python update(delta) for each script
     * @note Lua errors are caught and logged; Python errors propagate
     */
    static void Update(float delta);
};
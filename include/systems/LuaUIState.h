/**
 * @file LuaUIState.h
 * @brief Reactive UI state management backed by Lua
 */

#pragma once

#include <sol/sol.hpp>
#include <string>
#include <type_traits>
#include <memory>

#include "util/Logger.h"
#include "systems/ExpressionCache.h"

/**
 * @brief Reactive UI state manager backed by Lua VM
 *
 * Manages UI data loaded from Lua files with automatic change detection.
 * Provides path-based access to nested tables and expression evaluation.
 *
 * **State file structure:**
 * Lua files should return a table with UI data:
 * @code{.lua}
 * -- res/ui/state/game.lua
 * return {
 *     data = {
 *         fps = 60,
 *         showDebug = true,
 *         players = {
 *             {name = "Alice", score = 100},
 *             {name = "Bob", score = 85}
 *         }
 *     },
 *     computed = {
 *         -- Functions can be called from templates
 *         totalScore = function(self)
 *             local sum = 0
 *             for _, p in ipairs(self.data.players) do
 *                 sum = sum + p.score
 *             end
 *             return sum
 *         end
 *     }
 * }
 * @endcode
 *
 * **Path navigation:**
 * Uses dot notation to access nested values:
 * - `GetValue("data.fps")` returns sol::object with value 60
 * - `SetValue("data.fps", 120)` updates fps and marks dirty
 * - `GetValue("data.players")` returns Lua table
 *
 * **Change detection:**
 * - SetValue() automatically sets dirty flag
 * - ReactiveUI checks IsDirty() before re-rendering
 * - ClearDirty() called after render completes
 *
 * **Expression evaluation:**
 * - EvaluateCondition("data.showDebug") → true/false for v-if
 * - EvaluateAsString("data.fps") → "60" for {{ interpolation }}
 * - Full Lua expressions supported: "data.fps > 30"
 *
 * **Usage example:**
 * @code
 * LuaUIState state;
 * state.LoadStateFile("../res/ui/state/game.lua");
 *
 * // Read values
 * int fps = state.GetValue("data.fps").as<int>();
 *
 * // Update values (marks dirty)
 * state.SetValue("data.fps", 120);
 *
 * // Evaluate expressions
 * bool show = state.EvaluateCondition("data.showDebug");
 * std::string fpsStr = state.EvaluateAsString("data.fps");
 *
 * // Check if render needed
 * if (state.IsDirty()) {
 *     // Re-render UI
 *     state.ClearDirty();
 * }
 * @endcode
 *
 * @note Uses ScriptManager's shared Lua VM - state persists across loads
 * @note Not thread-safe - all operations must be on main thread
 * @see docs/architecture/UI_SYSTEM.md for state file conventions
 */
class LuaUIState
{
public:
    LuaUIState();
    ~LuaUIState() = default;

    /**
     * @brief Load UI state from Lua file
     * @param path Path to Lua file (e.g., "../res/ui/state/fps.lua")
     * @return true if loaded successfully, false on error
     * @note File must return a Lua table
     * @note Marks state as dirty and ready after successful load
     * @note Uses ScriptManager's Lua VM (must be initialized first)
     */
    bool LoadStateFile(const std::string &path);

    /**
     * @brief Get value from state using dot notation
     * @param key Path to value (e.g., "data.fps" or "data.players")
     * @return sol::object that can be converted to C++ types
     * @note Returns nil object if path not found
     * @note Use .as<T>() to convert: GetValue("data.fps").as<int>()
     */
    sol::object GetValue(const std::string &key);

    /**
     * @brief Set value in state using dot notation
     * @tparam T Value type (int, float, string, bool, sol::table, etc.)
     * @param key Path to value (e.g., "data.fps")
     * @param value New value to set
     * @note Automatically marks state as dirty for re-rendering
     * @note Creates nested tables if path doesn't exist
     * @note Example: SetValue("data.fps", 120) updates data.fps to 120
     */
    template <typename T>
    void SetValue(const std::string &key, const T &value);

    /**
     * @brief Set value without marking state as dirty
     * @tparam T Value type (int, float, string, bool, sol::table, etc.)
     * @param key Path to value (e.g., "viewportImage")
     * @param value New value to set
     * @note Does NOT trigger re-render - use for visual-only updates
     */
    template <typename T>
    void SetValueNoMarkDirty(const std::string &key, const T &value);

    /**
     * @brief Evaluate Lua expression as boolean condition
     * @param expression Lua expression (e.g., "data.showDebug" or "data.fps > 60")
     * @return true if expression evaluates to truthy value, false otherwise
     * @note Used by TemplateParser for v-if directive evaluation
     * @note Supports full Lua syntax: comparisons, logic, function calls
     */
    bool EvaluateCondition(const std::string &expression);

    /**
     * @brief Evaluate Lua expression and return as string
     * @param expression Lua expression (e.g., "data.fps")
     * @return String representation of expression result
     * @note Used by TemplateParser for {{ interpolation }} directive
     * @note Returns "nil" if expression fails or is nil
     * @note Numbers converted with tostring(), tables show address
     */
    std::string EvaluateAsString(const std::string &expression);

    /**
     * @brief Get entire state table
     * @return sol::table containing all state data
     * @note Used by TemplateParser for v-for iteration
     */
    sol::table GetStateTable() const { return m_stateTable; }

    /**
     * @brief Check if state has changed since last ClearDirty()
     * @return true if state modified, false if clean
     * @note ReactiveUI checks this to skip unnecessary re-renders
     */
    bool IsDirty() const { return m_isDirty; }

    /**
     * @brief Clear dirty flag after rendering
     * @note Called by ReactiveUI after GetRenderedHTML() completes
     */
    void ClearDirty() { m_isDirty = false; }

    /**
     * @brief Mark state as dirty to force re-render
     * @note Useful when Lua state modified outside of SetValue()
     */
    void MarkDirty() { m_isDirty = true; }

    /**
     * @brief Check if state file loaded successfully
     * @return true if LoadStateFile() succeeded, false if not loaded
     */
    bool IsReady() const { return m_isReady; }

private:
    /**
     * @brief Navigate nested tables using dot notation
     * @param path Dot-separated path (e.g., "data.players")
     * @return sol::object at path, or nil if not found
     * @note Internal helper for GetValue() and SetValue()
     * @note Handles arbitrary nesting depth
     */
    sol::object NavigatePath(const std::string &path);

    sol::state *m_lua;       ///< Reference to ScriptManager's Lua VM (not owned)
    sol::table m_stateTable; ///< Root state table loaded from file
    bool m_isDirty;          ///< Change detection flag
    bool m_isReady;          ///< LoadStateFile() success flag

    std::unique_ptr<ExpressionCache> m_exprCache; ///< Compiled expression cache (Phase 1)
};

// Template implementation for SetValue
template <typename T>
void LuaUIState::SetValue(const std::string &key, const T &value)
{
    if (!m_isReady)
    {
        LOG_ERROR("LuaUIState::SetValue called before state is ready");
        return;
    }

    // Get current value to check if it actually changed
    sol::object current = NavigatePath(key);

    // Check if value actually changed (skip re-render if unchanged)
    bool valueChanged = true;
    if constexpr (std::is_same_v<T, int>)
    {
        valueChanged = !current.is<int>() || current.as<int>() != value;
    }
    else if constexpr (std::is_same_v<T, double>)
    {
        valueChanged = !current.is<double>() || current.as<double>() != value;
    }
    else if constexpr (std::is_same_v<T, float>)
    {
        valueChanged = !current.is<double>() || static_cast<float>(current.as<double>()) != value;
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        valueChanged = !current.is<bool>() || current.as<bool>() != value;
    }
    else if constexpr (std::is_same_v<T, std::string>)
    {
        valueChanged = !current.is<std::string>() || current.as<std::string>() != value;
    }
    // For complex types (tables, etc.), assume changed (conservative)

    if (!valueChanged)
    {
        LOG_TRACE_L1("Value unchanged for key '{}'", key);
        return; // Early exit - no change needed
    }
    // Mark state as dirty otherwise
    m_isDirty = true;

    // Parse the key path (e.g., "data.fps" -> navigate to data table, set fps)
    size_t lastDot = key.rfind('.');

    if (lastDot == std::string::npos)
    {
        // Simple key, set directly on state table
        m_stateTable[key] = value;
    }
    else
    {
        // Nested key, navigate to parent table
        std::string parentPath = key.substr(0, lastDot);
        std::string finalKey = key.substr(lastDot + 1);

        sol::object parent = NavigatePath(parentPath);
        if (parent.is<sol::table>())
        {
            sol::table parentTable = parent.as<sol::table>();
            parentTable[finalKey] = value;
        }
    }

    LOG_TRACE_L1("Value set for key '{}'", key);
}

// Template implementation for SetValueNoMarkDirty
template <typename T>
void LuaUIState::SetValueNoMarkDirty(const std::string &key, const T &value)
{
    if (!m_isReady)
    {
        LOG_ERROR("LuaUIState::SetValueNoMarkDirty called before state is ready");
        return;
    }

    // Parse the key path (e.g., "data.fps" -> navigate to data table, set fps)
    size_t lastDot = key.rfind('.');

    if (lastDot == std::string::npos)
    {
        // Simple key, set directly on state table
        m_stateTable[key] = value;
    }
    else
    {
        // Nested key, navigate to parent table
        std::string parentPath = key.substr(0, lastDot);
        std::string finalKey = key.substr(lastDot + 1);

        sol::object parent = NavigatePath(parentPath);
        if (parent.is<sol::table>())
        {
            sol::table parentTable = parent.as<sol::table>();
            parentTable[finalKey] = value;
        }
    }

    // Note: NOT marking dirty - this is intentional for visual-only updates
}

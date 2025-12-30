#pragma once

#include <sol/sol.hpp>
#include <string>
#include <memory>

// Manages UI state loaded from Lua files
// Provides reactive state management for template directives
class LuaUIState {
public:
    LuaUIState();
    ~LuaUIState() = default;

    // Load UI state from a Lua file that returns a table
    // Example: return { data = { fps = 0 }, computed = { ... } }
    bool LoadStateFile(const std::string& path);

    // Get value from state using dot notation (e.g., "data.fps")
    // Returns sol::object which can be converted to various types
    sol::object GetValue(const std::string& key);

    // Set value in state using dot notation
    // Automatically marks state as dirty for re-rendering
    template<typename T>
    void SetValue(const std::string& key, const T& value);

    // Evaluate a Lua expression as a boolean condition (for v-if)
    // Example: "data.showDebug" or "data.fps > 60"
    bool EvaluateCondition(const std::string& expression);

    // Evaluate a Lua expression and return the result as a string
    // Example: "data.fps" returns "120"
    std::string EvaluateAsString(const std::string& expression);

    // Get the entire state table (for iteration in v-for)
    sol::table GetStateTable() const { return m_stateTable; }

    // Check if state has changed (for change detection)
    bool IsDirty() const { return m_isDirty; }

    // Clear dirty flag after rendering
    void ClearDirty() { m_isDirty = false; }

    // Check if state is loaded and ready
    bool IsReady() const { return m_isReady; }

private:
    // Navigate nested tables using dot notation
    // Example: "data.characters" -> returns table["data"]["characters"]
    sol::object NavigatePath(const std::string& path);

    // Reference to ScriptManager's Lua VM
    sol::state* m_lua;

    // The state table loaded from Lua file
    sol::table m_stateTable;

    // Track if state has changed
    bool m_isDirty;

    // Track if state file has been loaded
    bool m_isReady;
};

// Template implementation for SetValue
template<typename T>
void LuaUIState::SetValue(const std::string& key, const T& value) {
    if (!m_isReady) return;

    // Parse the key path (e.g., "data.fps" -> navigate to data table, set fps)
    size_t lastDot = key.rfind('.');

    if (lastDot == std::string::npos) {
        // Simple key, set directly on state table
        m_stateTable[key] = value;
    } else {
        // Nested key, navigate to parent table
        std::string parentPath = key.substr(0, lastDot);
        std::string finalKey = key.substr(lastDot + 1);

        sol::object parent = NavigatePath(parentPath);
        if (parent.is<sol::table>()) {
            sol::table parentTable = parent.as<sol::table>();
            parentTable[finalKey] = value;
        }
    }

    m_isDirty = true;
}

#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include "systems/LuaUIState.h"
#include "systems/TemplateParser.h"

// Simple reactive UI system that separates structure from data
// Renders HTML templates with dynamic values, only re-rendering when values change
// Supports Vue-style directives (v-if, v-for, {{}}) via Lua state management
class ReactiveUI {
public:
    static ReactiveUI& GetInstance() {
        static ReactiveUI instance;
        return instance;
    }

    ReactiveUI(ReactiveUI const&) = delete;
    void operator=(ReactiveUI const&) = delete;

    // === Legacy API (simple {{placeholder}} substitution) ===

    // Register a UI template with placeholder values
    // e.g., "FPS: {{fps}}, FrameTime: {{frameTime}}ms"
    void RegisterTemplate(const std::string& name, const std::string& htmlTemplate);

    // Set a value for a placeholder
    // Only triggers re-render if value actually changed
    template<typename T>
    void SetValue(const std::string& key, const T& value);

    // === New Lua-based API (v-if, v-for, {{}} directives) ===

    // Bind a Lua UI state for reactive data management
    void BindLuaState(std::shared_ptr<LuaUIState> state);

    // Register a template with Vue-style directives
    // Supports: v-if, v-for, {{expression}}
    void RegisterTemplateWithDirectives(const std::string& name, const std::string& htmlTemplate);

    // Get reference to bound Lua state (for updating values)
    std::shared_ptr<LuaUIState> GetLuaState() { return m_luaState; }

    // === Common API ===

    // Get the current rendered HTML (only re-renders if dirty)
    const std::string& GetRenderedHTML();

    // Force immediate re-render (useful for initial render)
    void ForceRender();

private:
    ReactiveUI() = default;

    // Legacy mode members
    std::string m_template;
    std::unordered_map<std::string, std::string> m_values;
    std::string m_cachedHTML;
    bool m_isDirty = true;

    // Lua-based mode members
    std::shared_ptr<LuaUIState> m_luaState;
    std::unique_ptr<TemplateParser> m_parser;
    bool m_useLuaMode = false;  // Track which rendering mode to use

    void RenderTemplate();
    void RenderWithLua();
};

// Template implementation
template<typename T>
void ReactiveUI::SetValue(const std::string& key, const T& value) {
    std::string valueStr = std::to_string(value);

    auto it = m_values.find(key);
    if (it == m_values.end() || it->second != valueStr) {
        m_values[key] = valueStr;
        m_isDirty = true;
    }
}

// Specialization for string
template<>
inline void ReactiveUI::SetValue<std::string>(const std::string& key, const std::string& value) {
    auto it = m_values.find(key);
    if (it == m_values.end() || it->second != value) {
        m_values[key] = value;
        m_isDirty = true;
    }
}

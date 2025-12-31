#include "systems/ReactiveUI.h"
#include "util/Logger.h"

// === Legacy API Implementation ===

void ReactiveUI::RegisterTemplate(const std::string& name, const std::string& htmlTemplate) {
    m_template = htmlTemplate;
    m_isDirty = true;
    LOG_INFO("[ReactiveUI] Template registered: {}", name);
}

const std::string& ReactiveUI::GetRenderedHTML() {
    if (m_useLuaMode) {
        // Lua-based rendering: check if Lua state is dirty
        if (m_luaState && m_luaState->IsDirty()) {
            RenderWithLua();
            m_luaState->ClearDirty();
        }
    } else {
        // Legacy rendering: check if local dirty flag is set
        if (m_isDirty) {
            RenderTemplate();
            m_isDirty = false;
        }
    }
    return m_cachedHTML;
}

void ReactiveUI::ForceRender() {
    if (m_useLuaMode) {
        RenderWithLua();
        if (m_luaState) {
            m_luaState->ClearDirty();
        }
    } else {
        m_isDirty = true;
        RenderTemplate();
        m_isDirty = false;
    }
}

void ReactiveUI::RenderTemplate() {
    LOG_DEBUG("[ReactiveUI] Rendering template (legacy mode, dirty)");

    m_cachedHTML = m_template;

    // Replace all {{key}} placeholders with their values
    for (const auto& [key, value] : m_values) {
        std::string placeholder = "{{" + key + "}}";
        size_t pos = 0;

        while ((pos = m_cachedHTML.find(placeholder, pos)) != std::string::npos) {
            m_cachedHTML.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }
}

// === Lua-based API Implementation ===

void ReactiveUI::BindLuaState(std::shared_ptr<LuaUIState> state) {
    m_luaState = state;
    m_useLuaMode = true;
    LOG_INFO("[ReactiveUI] Lua state bound, switching to Lua mode");
}

void ReactiveUI::RegisterTemplateWithDirectives(const std::string& name, const std::string& htmlTemplate) {
    if (!m_luaState) {
        LOG_ERROR("[ReactiveUI] Must bind Lua state before registering template with directives");
        return;
    }

    // Initialize parser if not already created
    if (!m_parser) {
        m_parser = std::make_unique<TemplateParser>();
    }

    // Parse the template and cache the parsed structure
    m_parser->Parse(htmlTemplate);
    m_useLuaMode = true;

    LOG_INFO("[ReactiveUI] Template with directives registered: {}", name);

    // Force initial render
    ForceRender();
}

void ReactiveUI::RenderWithLua() {
    if (!m_parser || !m_luaState) {
        LOG_ERROR("[ReactiveUI] Parser or Lua state not initialized");
        return;
    }

    LOG_DEBUG("[ReactiveUI] Rendering template (Lua mode, dirty)");

    // Use TemplateParser to evaluate directives with current Lua state
    m_cachedHTML = m_parser->Evaluate(*m_luaState);
}

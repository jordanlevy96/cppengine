#include "systems/ReactiveUI.h"
#include "controllers/ScriptManager.h"
#include "systems/HTMLRendererMT.h"
#include "util/Logger.h"
#include <fstream>
#include <sstream>

// === Legacy API Implementation ===

void ReactiveUI::RegisterTemplate(const std::string &name, const std::string &htmlTemplate)
{
    m_template = htmlTemplate;
    m_isDirty = true;
    LOG_INFO("[ReactiveUI] Template registered: {}", name);
}

const std::string &ReactiveUI::GetRenderedHTML()
{
    if (m_useLuaMode)
    {
        // Lua-based rendering: check if Lua state is dirty
        if (m_luaState && m_luaState->IsDirty())
        {
            RenderWithLua();
            m_luaState->ClearDirty();
        }
    }
    else
    {
        // Legacy rendering: check if local dirty flag is set
        if (m_isDirty)
        {
            RenderTemplate();
            m_isDirty = false;
        }
    }
    return m_cachedHTML;
}

void ReactiveUI::ForceRender()
{
    if (m_useLuaMode)
    {
        RenderWithLua();
        if (m_luaState)
        {
            m_luaState->ClearDirty();
        }
    }
    else
    {
        m_isDirty = true;
        RenderTemplate();
        m_isDirty = false;
    }
}

void ReactiveUI::RenderTemplate()
{
    LOG_DEBUG("[ReactiveUI] Rendering template (legacy mode, dirty)");

    m_cachedHTML = m_template;

    // Replace all {{key}} placeholders with their values
    for (const auto &[key, value] : m_values)
    {
        std::string placeholder = "{{" + key + "}}";
        size_t pos = 0;

        while ((pos = m_cachedHTML.find(placeholder, pos)) != std::string::npos)
        {
            m_cachedHTML.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }
}

// === Lua-based API Implementation ===

void ReactiveUI::BindLuaState(std::shared_ptr<LuaUIState> state)
{
    m_luaState = state;
    m_useLuaMode = true;
    LOG_INFO("[ReactiveUI] Lua state bound, switching to Lua mode");
}

void ReactiveUI::RegisterTemplateWithDirectives(const std::string &name, const std::string &htmlTemplate)
{
    if (!m_luaState)
    {
        LOG_ERROR("[ReactiveUI] Must bind Lua state before registering template with directives");
        return;
    }

    // Initialize parser if not already created
    if (!m_parser)
    {
        m_parser = std::make_unique<TemplateParser>();
    }

    // Parse the template and cache the parsed structure
    m_parser->Parse(htmlTemplate);
    m_useLuaMode = true;

    LOG_INFO("[ReactiveUI] Template with directives registered: {}", name);

    // Force initial render
    ForceRender();
}

void ReactiveUI::RenderWithLua()
{
    if (!m_parser || !m_luaState)
    {
        LOG_ERROR("[ReactiveUI] Parser or Lua state not initialized");
        return;
    }

    LOG_DEBUG("[ReactiveUI] Rendering template (Lua mode, dirty)");

    // Use TemplateParser to evaluate directives with current Lua state
    m_cachedHTML = m_parser->Evaluate(*m_luaState);

    // Update HTML renderer with new event handlers after re-render
    HTMLRendererMT &htmlRenderer = HTMLRendererMT::GetInstance();
    htmlRenderer.SetEventHandlers(m_parser->GetEventHandlers());
    LOG_DEBUG("[ReactiveUI] Updated {} event handlers after render", m_parser->GetEventHandlers().size());
}

// === Event Handling Implementation ===

void ReactiveUI::DispatchEvent(const std::string &eventType,
                               const std::string &handlerExpr,
                               const EventData &eventData)
{
    if (!m_luaState || !m_luaState->IsReady())
    {
        LOG_ERROR("[ReactiveUI] Lua state not ready for event dispatch");
        return;
    }

    LOG_DEBUG("[ReactiveUI] Dispatching {} event: handler='{}', elem='{}'",
              eventType, handlerExpr, eventData.elemId);

    // Parse handler expression: "methodName" or "methodName(args)" or "methodName($event)"
    std::string handlerName;
    std::vector<std::string> args;

    size_t parenPos = handlerExpr.find('(');
    if (parenPos == std::string::npos)
    {
        // Simple handler: "methodName"
        handlerName = handlerExpr;
    }
    else
    {
        // Handler with args: "methodName(arg1, arg2)"
        handlerName = handlerExpr.substr(0, parenPos);

        size_t closePos = handlerExpr.find(')');
        if (closePos != std::string::npos)
        {
            std::string argsStr = handlerExpr.substr(parenPos + 1, closePos - parenPos - 1);

            // Simple arg parsing (handles single arg for now)
            if (!argsStr.empty())
            {
                // Trim whitespace
                size_t start = argsStr.find_first_not_of(" \t");
                size_t end = argsStr.find_last_not_of(" \t");
                if (start != std::string::npos)
                {
                    args.push_back(argsStr.substr(start, end - start + 1));
                }
            }
        }
    }

    // Get methods table from Lua state
    sol::object methodsObj = m_luaState->GetValue("methods");
    if (!methodsObj.is<sol::table>())
    {
        LOG_ERROR("[ReactiveUI] No 'methods' table found in Lua state");
        return;
    }

    sol::table methods = methodsObj.as<sol::table>();
    sol::object handlerObj = methods[handlerName];
    if (!handlerObj.is<sol::function>())
    {
        LOG_ERROR("[ReactiveUI] Handler '{}' not found in methods table", handlerName);
        return;
    }

    sol::function handler = handlerObj.as<sol::function>();
    sol::table stateTable = m_luaState->GetStateTable();

    // Call handler with appropriate arguments
    sol::protected_function_result result;
    try
    {
        if (args.empty())
        {
            // No args: handler(self)
            result = handler(stateTable);
        }
        else if (args[0] == "$event")
        {
            // Event object: handler(self, event)
            // Create event table using ScriptManager's Lua state
            sol::state &lua = ScriptManager::GetInstance().GetLuaState();
            sol::table eventTable = lua.create_table();
            eventTable["x"] = eventData.x;
            eventTable["y"] = eventData.y;
            eventTable["button"] = eventData.button;
            eventTable["elemId"] = eventData.elemId;
            eventTable["type"] = eventData.eventType;
            result = handler(stateTable, eventTable);
        }
        else
        {
            // Literal arg: handler(self, arg)
            // Try to parse as number first, otherwise use as string
            try
            {
                int numArg = std::stoi(args[0]);
                result = handler(stateTable, numArg);
            }
            catch (...)
            {
                // Not a number, use as string (remove quotes if present)
                std::string strArg = args[0];
                if (strArg.front() == '\'' || strArg.front() == '"')
                {
                    strArg = strArg.substr(1, strArg.length() - 2);
                }
                result = handler(stateTable, strArg);
            }
        }

        if (!result.valid())
        {
            sol::error err = result;
            LOG_ERROR("[ReactiveUI] Lua handler '{}' failed: {}", handlerName, err.what());
            return;
        }

        LOG_INFO("[ReactiveUI] Handler '{}' executed successfully", handlerName);
    }
    catch (const sol::error &e)
    {
        LOG_ERROR("[ReactiveUI] Exception calling handler '{}': {}", handlerName, e.what());
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("[ReactiveUI] Exception calling handler '{}': {}", handlerName, e.what());
    }
}

// === Template Loading Implementation ===

std::string ReactiveUI::LoadTextFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        LOG_ERROR("[ReactiveUI] Failed to open file: {}", path);
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string ReactiveUI::LoadTemplateFromFiles(
    const std::string &templatePath,
    const std::string &cssPath)
{
    // Load CSS
    std::string cssContent = LoadTextFile(cssPath);
    if (cssContent.empty())
    {
        LOG_ERROR("[ReactiveUI] Failed to load CSS from: {}", cssPath);
        return "";
    }

    // Load HTML template
    std::string htmlTemplate = LoadTextFile(templatePath);
    if (htmlTemplate.empty())
    {
        LOG_ERROR("[ReactiveUI] Failed to load template from: {}", templatePath);
        return "";
    }

    // Inject CSS into template
    const std::string placeholder = "<!-- CSS_PLACEHOLDER -->";
    size_t pos = htmlTemplate.find(placeholder);

    if (pos == std::string::npos)
    {
        LOG_WARNING("[ReactiveUI] CSS placeholder not found in template, appending to <head>");
        // Fallback: inject before </head>
        size_t headEnd = htmlTemplate.find("</head>");
        if (headEnd != std::string::npos)
        {
            std::string styleTag = "    <style>\n" + cssContent + "    </style>\n";
            htmlTemplate.insert(headEnd, styleTag);
        }
        else
        {
            LOG_ERROR("[ReactiveUI] Invalid HTML structure - no </head> tag found");
            return "";
        }
    }
    else
    {
        // Replace placeholder with CSS
        std::string styleTag = "<style>\n" + cssContent + "    </style>";
        htmlTemplate.replace(pos, placeholder.length(), styleTag);
    }

    LOG_INFO("[ReactiveUI] Loaded template ({} bytes HTML, {} bytes CSS)",
             htmlTemplate.size(), cssContent.size());

    return htmlTemplate;
}

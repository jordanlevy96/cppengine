/**
 * @file Editor.cpp
 * @brief Editor implementation - Phase 2
 */

#include "editor/Editor.h"
#include "util/Logger.h"
#include "util/ConfigLoader.h"
#include "util/FrameTiming.h"
#include "systems/LuaUIState.h"
#include "systems/ScriptSystem.h"
#include "systems/TweenSystem.h"
#include "systems/HierarchySystem.h"
#include "systems/TemplateParser.h"
#include "controllers/Game.h"
#include "controllers/ScriptManager.h"
#include "util/FileIO.h"

#include <sol/sol.hpp>
#include <glm/gtc/quaternion.hpp>
#include <fstream>
#include <filesystem>

bool Editor::Initialize()
{
    LOG_INFO("[Editor] Starting initialization...");

    if (m_initialized)
    {
        LOG_WARNING("[Editor] Already initialized");
        return true;
    }

    // Load config and initialize EngineCore
    if (!m_core.Initialize("../res/conf/settings.yaml", conf, &m_camera))
    {
        return false;
    }

    // Get references to subsystems
    m_windowManager = m_core.GetWindowManager();
    m_htmlRenderer = m_core.GetHTMLRenderer();
    m_registry = m_core.GetRegistry();
    m_reactiveUI = m_core.GetReactiveUI();

    // Initialize scene viewport (800x600 for now) with camera from EngineCore
    m_viewport = new SceneViewport();
    Camera *camera = m_core.GetCamera();
    if (!m_viewport->Initialize(800, 600, camera))
    {
        LOG_CRITICAL("Failed to initialize SceneViewport");
        return false;
    }

    // Load scene (editor test scene without game scripts)
    if (!m_core.LoadScene("scenes/EditorTest.yaml"))
    {
        LOG_ERROR("Failed to load scene: scenes/EditorTest.yaml");
        return false;
    }

    // Load editor UI (creates Lua state)
    LoadEditorUI();

    // Initialize UI Editor (populates templates in Lua state)
    InitializeUIEditor();

    // Force UI re-render to show templates in dropdown
    if (m_reactiveUI && m_reactiveUI->GetLuaState())
    {
        m_reactiveUI->GetLuaState()->MarkDirty();
        std::string renderedHTML = m_reactiveUI->GetRenderedHTML();
        m_htmlRenderer->UpdateHTML(renderedHTML);
    }

    // Populate scene tree with entities from Registry
    UpdateSceneTree();

    // Register keyboard input handler for editor shortcuts and text input
    m_keyboardHandlerId = m_windowManager->RegisterInputHandler([this](const InputEvent &event)
    {
        if (event.type == InputTypes::Char)
        {
            return HandleTextInput(event);
        }
        return HandleEditorInput(event);
    });

    // Register click input handler for UI interactions
    m_clickHandlerId = m_windowManager->RegisterInputHandler([this](const InputEvent &event)
    {
        if (event.type != InputTypes::Click)
        {
            return false;
        }

        // Extract click payload (platforms may use vec2 or vec3)
        try
        {
            if (std::holds_alternative<glm::vec3>(event.input))
            {
                auto click = std::get<glm::vec3>(event.input);
                return HandleUIClick(click.x, click.y, static_cast<int>(click.z));
            }
            else if (std::holds_alternative<glm::vec2>(event.input))
            {
                auto click = std::get<glm::vec2>(event.input);
                return HandleUIClick(click.x, click.y, 0); // Default to left button
            }
            else
            {
                LOG_WARNING("[Editor] Click event has unexpected payload type");
                return false;
            }
        }
        catch (const std::bad_variant_access &)
        {
            LOG_WARNING("[Editor] Failed to read click event payload");
            return false;
        }
    });

    // Register selectEntity function for Lua event binding in ReactiveUI's state
    std::shared_ptr<LuaUIState> luaUIState = m_reactiveUI->GetLuaState();

    // Get Lua state and create methods table for ReactiveUI event dispatching
    sol::table stateTable = luaUIState->GetStateTable();

    // Get or create methods table (defensive - editor.lua should have it, but be safe)
    sol::table methods = stateTable["methods"];
    if (!methods.valid())
    {
        ScriptManager &scriptManager = ScriptManager::GetInstance();
        sol::state &lua = scriptManager.GetLuaState();
        methods = lua.create_table();
        stateTable["methods"] = methods;
        LOG_WARNING("[Editor] Created methods table - it should exist in editor.lua");
    }

    // Register selectEntity in the methods table - bind to C++ implementation
    methods["selectEntity"] = [this](sol::table self, uint32_t entityId)
    {
        SelectEntity(static_cast<EntityID>(entityId));
    };

    // Register UI Editor methods
    methods["loadTemplate"] = [this](sol::table self, const std::string &templateName)
    {
        LoadUITemplate(templateName);
    };

    methods["onCodeChange"] = [this](sol::table self)
    {
        // Mark preview as needing update (debounced)
        m_previewNeedsUpdate = true;
        m_lastCodeChange = std::chrono::steady_clock::now();
    };

    methods["saveTemplate"] = [this](sol::table self)
    {
        SaveUITemplate();
    };

    methods["previewTemplate"] = [this](sol::table self)
    {
        UpdatePreview();
    };

    methods["createTemplate"] = [this](sol::table self, const std::string &templateName)
    {
        CreateUITemplate(templateName);
    };

    methods["focusInput"] = [this](sol::table self, const std::string &fieldPath)
    {
        m_focusedInputField = fieldPath;
    };

    luaUIState->SetValue("methods", methods);

    m_initialized = true;
    return true;
}

void Editor::Run()
{
    if (!m_initialized)
    {
        LOG_ERROR("Cannot run editor - not initialized");
        return;
    }

    LOG_INFO("Starting editor main loop...");

    // Initialize frame timing (SIMPLE mode: delta-only, no fixed timestep)
    FrameTiming timing(FrameTimingMode::SIMPLE, 60.0);

    while (!glfwWindowShouldClose(m_windowManager->window) && !m_shouldClose)
    {
        // CRITICAL: Poll input events first so window is responsive
        glfwPollEvents();

        // Update frame timing (calculates delta)
        timing.Update();
        m_delta = timing.GetDelta();

        // Update game systems (tweens, tweens + hierarchy)
        // Note: ScriptSystem is deliberately skipped in editor mode
        UpdateSystems(m_delta);

        // Update UI Editor (handles debounced preview updates)
        UpdateUIEditor(m_delta);

        // Render editor frame (viewport + UI)
        Render();

        // Swap buffers
        glfwSwapBuffers(m_windowManager->window);
    }

    LOG_INFO("Editor main loop ended");
}

void Editor::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    LOG_INFO("Shutting down editor...");

    // Unregister input handlers
    if (m_keyboardHandlerId != 0)
    {
        m_windowManager->UnregisterInputHandler(m_keyboardHandlerId);
        m_keyboardHandlerId = 0;
    }
    if (m_clickHandlerId != 0)
    {
        m_windowManager->UnregisterInputHandler(m_clickHandlerId);
        m_clickHandlerId = 0;
    }

    // Shutdown systems (in reverse order of initialization)
    if (m_viewport)
    {
        m_viewport->Shutdown();
        delete m_viewport;
        m_viewport = nullptr;
    }
    m_htmlRenderer->Shutdown();
    m_windowManager->Shutdown();

    m_initialized = false;
    LOG_INFO("Editor shutdown complete");
}

void Editor::Render()
{
    // Render scene to viewport FBO
    m_viewport->Render();

    // Get viewport texture as data URI and update Lua state
    // Note: This is expensive (~5-10ms) but necessary for now
    std::string viewportDataURI = m_viewport->GetTextureAsDataURI();

    if (m_reactiveUI && m_reactiveUI->GetLuaState())
    {
        if (!viewportDataURI.empty())
        {
            // Update viewport image - this will mark dirty but that's needed
            // to re-render the viewport in the HTML
            m_reactiveUI->GetLuaState()->SetValue("viewportImage", viewportDataURI);
        }
    }

    // Update HTML (GetRenderedHTML re-renders if dirty)
    if (m_reactiveUI)
    {
        std::string renderedHTML = m_reactiveUI->GetRenderedHTML();
        m_htmlRenderer->UpdateHTML(renderedHTML);
    }

    // Ensure viewport matches framebuffer (handles Retina/HiDPI)
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_windowManager->window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    // Clear screen
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render editor UI
    m_htmlRenderer->Render();
}

void Editor::LoadEditorUI()
{
    // Load editor Lua state from file
    auto luaState = std::make_shared<LuaUIState>();
    if (!luaState->LoadStateFile("../res/ui/state/editor.lua"))
    {
        LOG_ERROR("Failed to load editor Lua state");
        return;
    }

    m_reactiveUI->BindLuaState(luaState);

    // Load editor HTML template from file
    std::ifstream htmlFile("../res/ui/editor.html");
    if (!htmlFile.is_open())
    {
        LOG_ERROR("Failed to open editor HTML template: ../res/ui/editor.html");
        return;
    }

    std::string editorHTML((std::istreambuf_iterator<char>(htmlFile)), std::istreambuf_iterator<char>());
    htmlFile.close();

    // Register template with ReactiveUI so directives (v-for, v-if, etc.) are processed
    m_reactiveUI->RegisterTemplateWithDirectives("EditorUI", editorHTML);

    // Get the rendered HTML and send to HTMLRenderer
    std::string renderedHTML = m_reactiveUI->GetRenderedHTML();
    m_htmlRenderer->LoadHTML(renderedHTML);
}

void Editor::UpdateSceneTree()
{
    if (!m_reactiveUI || !m_reactiveUI->GetLuaState())
    {
        LOG_WARNING("Cannot update scene tree - ReactiveUI or LuaState not available");
        return;
    }

    // Get all entities from Registry
    std::vector<EntityID> entityIds = m_registry->GetAllEntities();

    // Get Lua state directly from ScriptManager
    ScriptManager &scriptManager = ScriptManager::GetInstance();
    sol::state &lua = scriptManager.GetLuaState();

    // Get Lua UI state for SetValue
    auto luaState = m_reactiveUI->GetLuaState();

    // Create entities array as a Lua table (using Lua state directly)
    sol::table entitiesTable = lua.create_table();

    for (size_t i = 0; i < entityIds.size(); i++)
    {
        EntityID id = entityIds[i];
        std::string name = m_registry->GetEntityName(id);

        // Create entity table (using Lua state directly)
        sol::table entityTable = lua.create_table();
        entityTable["id"] = static_cast<uint32_t>(id); // Cast to Lua-safe size
        entityTable["name"] = name;

        // Get parent ID from HierarchyComponent (if exists)
        if (m_registry->HasComponent<HierarchyComponent>(id))
        {
            EntityID parent = m_registry->GetComponent<HierarchyComponent>(id).Parent;
            entityTable["parent"] = static_cast<uint32_t>(parent); // Cast to Lua-safe size
        }
        // Leave parent as nil in Lua if no HierarchyComponent

        // Add to entities array (Lua arrays are 1-indexed)
        entitiesTable[i + 1] = entityTable;
    }

    // Set entities table in state
    luaState->SetValue("entities", entitiesTable);
    luaState->MarkDirty();
}

void Editor::UpdateSystems(double deltaTime)
{
    // Note: ScriptSystem::Update() is skipped in editor mode to avoid requiring
    // game-specific Lua EventQueue setup. Scripts should be tested in the game runtime.

    // Update animation system (tweens)
    TweenSystem::Update(deltaTime);

    // Update transform hierarchy (for parenting)
    HierarchySystem::Update();
}

bool Editor::HandleTextInput(const InputEvent &event)
{
    if (event.type != InputTypes::Char)
    {
        return false;
    }

    // Get character from event
    if (!std::holds_alternative<std::string>(event.input))
    {
        return false;
    }

    std::string ch = std::get<std::string>(event.input);
    if (ch.empty())
    {
        return false;
    }

    // If we have a focused input field, update its value
    if (!m_focusedInputField.empty() && m_reactiveUI && m_reactiveUI->GetLuaState())
    {
        auto luaState = m_reactiveUI->GetLuaState();
        
        // Get current value
        sol::object currentValueObj = luaState->GetValue(m_focusedInputField);
        std::string currentValue = "";
        if (currentValueObj.valid() && currentValueObj.get_type() == sol::type::string)
        {
            currentValue = currentValueObj.as<std::string>();
        }

        // Handle special characters
        if (ch == "\b" || ch == "\x7f") // Backspace
        {
            if (!currentValue.empty())
            {
                currentValue.pop_back();
            }
        }
        else if (ch == "\r" || ch == "\n") // Enter - don't add to input
        {
            return true; // Consume but don't update
        }
        else
        {
            // Append character
            currentValue += ch;
        }

        // Update value in Lua state
        luaState->SetValue(m_focusedInputField, currentValue);
        luaState->MarkDirty();

        return true; // Consume event
    }

    return false; // No focused input field, pass through
}

bool Editor::HandleEditorInput(const InputEvent &event)
{
    // Only handle keyboard events
    if (event.type != InputTypes::Key)
    {
        return false; // Pass through to Lua
    }

    // Get key name from variant
    std::string key = std::get<std::string>(event.input);

    // ESC - Quit editor
    if (key == "ESCAPE" && event.mods == 0)
    {
        LOG_INFO("[Editor] ESC pressed - closing editor");
        m_shouldClose = true;
        return true; // Consume event
    }

    // Ctrl+S - Save scene (future)
    if (key == "S" && (event.mods & GLFW_MOD_CONTROL))
    {
        LOG_INFO("[Editor] Ctrl+S pressed - save not implemented yet");
        // TODO: Implement scene saving
        return true;
    }

    // Ctrl+O - Open scene (future)
    if (key == "O" && (event.mods & GLFW_MOD_CONTROL))
    {
        LOG_INFO("[Editor] Ctrl+O pressed - open not implemented yet");
        // TODO: Implement scene loading
        return true;
    }

    // Ctrl+Z - Undo (future)
    if (key == "Z" && (event.mods & GLFW_MOD_CONTROL) && !(event.mods & GLFW_MOD_SHIFT))
    {
        LOG_INFO("[Editor] Ctrl+Z pressed - undo not implemented yet");
        // TODO: Implement undo
        return true;
    }

    // Ctrl+Shift+Z - Redo (future)
    if (key == "Z" && (event.mods & GLFW_MOD_CONTROL) && (event.mods & GLFW_MOD_SHIFT))
    {
        LOG_INFO("[Editor] Ctrl+Shift+Z pressed - redo not implemented yet");
        // TODO: Implement redo
        return true;
    }

    // Event not handled by editor, pass to Lua
    return false;
}

void Editor::SelectEntity(EntityID entityId)
{
    m_selectedEntityId = entityId;

    if (m_viewport)
    {
        m_viewport->SetSelectedEntity(entityId);
    }

    UpdateInspector();

    if (m_reactiveUI && m_reactiveUI->GetLuaState())
    {
        m_reactiveUI->GetLuaState()->MarkDirty();
    }
}

void Editor::UpdateInspector()
{
    if (!m_reactiveUI || !m_reactiveUI->GetLuaState())
    {
        return;
    }

    auto luaState = m_reactiveUI->GetLuaState();

    if (m_selectedEntityId == ENTITY_NULL)
    {
        // Clear selection
        luaState->SetValue("selectedEntityId", nullptr);
        luaState->SetValue("selectedEntityName", "");
        return;
    }

    // Get entity name
    std::string entityName = m_registry->GetEntityName(m_selectedEntityId);
    luaState->SetValue("selectedEntityId", static_cast<uint32_t>(m_selectedEntityId));
    luaState->SetValue("selectedEntityName", entityName);

    // Get Transform component
    ScriptManager &scriptManager = ScriptManager::GetInstance();
    sol::state &lua = scriptManager.GetLuaState();

    if (m_registry->HasComponent<Transform>(m_selectedEntityId))
    {
        const Transform &transform = m_registry->GetComponent<Transform>(m_selectedEntityId);

        // Set position
        sol::table posTable = lua.create_table();
        posTable["x"] = transform.Pos.x;
        posTable["y"] = transform.Pos.y;
        posTable["z"] = transform.Pos.z;
        luaState->SetValue("selectedEntityPosition", posTable);

        // Convert quaternion to Euler angles for display
        glm::vec3 euler = glm::eulerAngles(transform.Rotation) * glm::degrees(1.0f);
        sol::table rotTable = lua.create_table();
        rotTable["x"] = euler.x;
        rotTable["y"] = euler.y;
        rotTable["z"] = euler.z;
        luaState->SetValue("selectedEntityRotation", rotTable);

        // Set scale
        sol::table scaleTable = lua.create_table();
        scaleTable["x"] = transform.Scale.x;
        scaleTable["y"] = transform.Scale.y;
        scaleTable["z"] = transform.Scale.z;
        luaState->SetValue("selectedEntityScale", scaleTable);
    }
    else
    {
        // No Transform component - show defaults
        sol::table posTable = lua.create_table();
        posTable["x"] = 0;
        posTable["y"] = 0;
        posTable["z"] = 0;
        luaState->SetValue("selectedEntityPosition", posTable);

        sol::table rotTable = lua.create_table();
        rotTable["x"] = 0;
        rotTable["y"] = 0;
        rotTable["z"] = 0;
        luaState->SetValue("selectedEntityRotation", rotTable);

        sol::table scaleTable = lua.create_table();
        scaleTable["x"] = 1;
        scaleTable["y"] = 1;
        scaleTable["z"] = 1;
        luaState->SetValue("selectedEntityScale", scaleTable);
    }
}

bool Editor::HandleUIClick(float x, float y, int button)
{
    if (!m_htmlRenderer)
    {
        return false;
    }

    // Pass click to HTMLRenderer for hit-testing and event dispatch
    return m_htmlRenderer->HandleClickEvent(x, y, button);
}

void Editor::InitializeUIEditor()
{
    // Initialize template manager
    m_templateManager = &UITemplateManager::GetInstance();
    m_templateManager->Initialize("../res/ui/templates/", "../res/ui/styles/", "../res/ui/state/");

    // Populate template list in Lua state
    auto luaState = m_reactiveUI->GetLuaState();
    if (!luaState)
    {
        LOG_ERROR("[Editor] Cannot initialize UI Editor - Lua state not available");
        return;
    }

    ScriptManager &scriptManager = ScriptManager::GetInstance();
    sol::state &lua = scriptManager.GetLuaState();

    std::vector<TemplateInfo> templates = m_templateManager->ListTemplates();
    sol::table templatesTable = lua.create_table();

    for (size_t i = 0; i < templates.size(); i++)
    {
        sol::table tplTable = lua.create_table();
        tplTable["name"] = templates[i].name;
        tplTable["hasHTML"] = templates[i].hasHTML;
        tplTable["hasCSS"] = templates[i].hasCSS;
        tplTable["hasLua"] = templates[i].hasLua;
        templatesTable[i + 1] = tplTable; // Lua arrays are 1-indexed
    }

    luaState->SetValue("uiEditor.templates", templatesTable);
    luaState->MarkDirty();

    if (templates.size() > 0)
    {
        LOG_INFO("[Editor] UI Editor: {} template(s) loaded", templates.size());
    }
}

void Editor::UpdateUIEditor(double deltaTime)
{
    if (!m_previewNeedsUpdate)
    {
        return;
    }

    // Check if debounce time has elapsed
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastCodeChange);

    if (elapsed.count() >= PREVIEW_DEBOUNCE_MS)
    {
        UpdatePreview();
        m_previewNeedsUpdate = false;
    }
}

void Editor::LoadUITemplate(const std::string &templateName)
{
    if (!m_templateManager)
    {
        LOG_ERROR("[Editor] Cannot load template - template manager not initialized");
        return;
    }

    std::string html, css, lua;
    if (!m_templateManager->LoadTemplate(templateName, html, css, lua))
    {
        LOG_ERROR("[Editor] Failed to load template: {}", templateName);
        auto luaState = m_reactiveUI->GetLuaState();
        if (luaState)
        {
            luaState->SetValue("uiEditor.errorMessage", "Failed to load template: " + templateName);
            luaState->MarkDirty();
        }
        return;
    }

    // Update Lua state with template content
    auto luaState = m_reactiveUI->GetLuaState();
    if (luaState)
    {
        luaState->SetValue("uiEditor.currentTemplate", templateName);
        luaState->SetValue("uiEditor.htmlCode", html);
        luaState->SetValue("uiEditor.cssCode", css);
        luaState->SetValue("uiEditor.luaCode", lua);
        luaState->SetValue("uiEditor.isDirty", false);
        luaState->SetValue("uiEditor.errorMessage", "");
        luaState->MarkDirty();

        // Update preview immediately
        UpdatePreview();
    }

    LOG_INFO("[Editor] Loaded template: {}", templateName);
}

void Editor::SaveUITemplate()
{
    if (!m_templateManager)
    {
        LOG_ERROR("[Editor] Cannot save template - template manager not initialized");
        return;
    }

    auto luaState = m_reactiveUI->GetLuaState();
    if (!luaState)
    {
        LOG_ERROR("[Editor] Cannot save template - Lua state not available");
        return;
    }

    // Get current template name
    sol::object currentTemplateObj = luaState->GetValue("uiEditor.currentTemplate");
    if (!currentTemplateObj.valid() || currentTemplateObj.get_type() != sol::type::string)
    {
        LOG_ERROR("[Editor] Cannot save - no template selected");
        luaState->SetValue("uiEditor.errorMessage", "No template selected");
        luaState->MarkDirty();
        return;
    }

    std::string templateName = currentTemplateObj.as<std::string>();
    if (templateName.empty())
    {
        LOG_ERROR("[Editor] Cannot save - template name is empty");
        luaState->SetValue("uiEditor.errorMessage", "Template name is empty");
        luaState->MarkDirty();
        return;
    }

    // Get code content from Lua state
    sol::object htmlObj = luaState->GetValue("uiEditor.htmlCode");
    sol::object cssObj = luaState->GetValue("uiEditor.cssCode");
    sol::object luaObj = luaState->GetValue("uiEditor.luaCode");

    std::string html = htmlObj.valid() && htmlObj.get_type() == sol::type::string ? htmlObj.as<std::string>() : "";
    std::string css = cssObj.valid() && cssObj.get_type() == sol::type::string ? cssObj.as<std::string>() : "";
    std::string lua = luaObj.valid() && luaObj.get_type() == sol::type::string ? luaObj.as<std::string>() : "";

    // Save template
    if (m_templateManager->SaveTemplate(templateName, html, css, lua))
    {
        LOG_INFO("[Editor] Saved template: {}", templateName);
        luaState->SetValue("uiEditor.isDirty", false);
        luaState->SetValue("uiEditor.errorMessage", "");
    }
    else
    {
        LOG_ERROR("[Editor] Failed to save template: {}", templateName);
        luaState->SetValue("uiEditor.errorMessage", "Failed to save template");
    }

    luaState->MarkDirty();
}

void Editor::CreateUITemplate(const std::string &templateName)
{
    if (!m_templateManager)
    {
        LOG_ERROR("[Editor] Cannot create template - template manager not initialized");
        return;
    }

    if (templateName.empty())
    {
        LOG_ERROR("[Editor] Cannot create template - name is empty");
        return;
    }

    if (m_templateManager->TemplateExists(templateName))
    {
        LOG_WARNING("[Editor] Template already exists: {}", templateName);
        auto luaState = m_reactiveUI->GetLuaState();
        if (luaState)
        {
            luaState->SetValue("uiEditor.errorMessage", "Template already exists: " + templateName);
            luaState->MarkDirty();
        }
        return;
    }

    if (m_templateManager->CreateTemplate(templateName))
    {
        LOG_INFO("[Editor] Created template: {}", templateName);

        // Reload template list
        InitializeUIEditor();

        // Load the newly created template
        LoadUITemplate(templateName);
    }
    else
    {
        LOG_ERROR("[Editor] Failed to create template: {}", templateName);
        auto luaState = m_reactiveUI->GetLuaState();
        if (luaState)
        {
            luaState->SetValue("uiEditor.errorMessage", "Failed to create template");
            luaState->MarkDirty();
        }
    }
}

void Editor::UpdatePreview()
{
    if (!m_reactiveUI)
    {
        return;
    }

    auto luaState = m_reactiveUI->GetLuaState();
    if (!luaState)
    {
        return;
    }

    // Get code content from Lua state
    sol::object htmlObj = luaState->GetValue("uiEditor.htmlCode");
    sol::object cssObj = luaState->GetValue("uiEditor.cssCode");
    sol::object luaObj = luaState->GetValue("uiEditor.luaCode");

    std::string html = htmlObj.valid() && htmlObj.get_type() == sol::type::string ? htmlObj.as<std::string>() : "";
    std::string css = cssObj.valid() && cssObj.get_type() == sol::type::string ? cssObj.as<std::string>() : "";
    std::string lua = luaObj.valid() && luaObj.get_type() == sol::type::string ? luaObj.as<std::string>() : "";

    if (html.empty())
    {
        luaState->SetValue("uiEditor.previewHTML", "<p>No HTML content to preview</p>");
        luaState->SetValue("uiEditor.errorMessage", "");
        luaState->MarkDirty();
        return;
    }

    try
    {
        // Inject CSS into HTML
        std::string htmlWithCSS = html;
        size_t stylePos = htmlWithCSS.find("</head>");
        if (stylePos != std::string::npos)
        {
            std::string styleTag = "    <style>\n" + css + "    </style>\n";
            htmlWithCSS.insert(stylePos, styleTag);
        }
        else
        {
            // No </head> tag, prepend with style
            htmlWithCSS = "<style>\n" + css + "</style>\n" + htmlWithCSS;
        }

        // Try to process template directives if Lua code is provided
        std::string processedHTML = htmlWithCSS;

        if (!lua.empty())
        {
            try
            {
                ScriptManager &scriptManager = ScriptManager::GetInstance();
                sol::state &luaVM = scriptManager.GetLuaState();

                // Execute Lua code to get state table
                sol::load_result loadResult = luaVM.load(lua);
                if (loadResult.valid())
                {
                    sol::protected_function_result result = loadResult();
                    if (result.valid() && result.return_count() > 0 && result[0].is<sol::table>())
                    {
                        // TODO: Add SetStateTable to LuaUIState to avoid temp file workaround
                        std::string tempLuaFile = "../res/ui/state/.preview_temp.lua";
                        FileIO::WriteTextFile(tempLuaFile, lua);

                        TemplateParser parser;
                        parser.Parse(htmlWithCSS);

                        auto previewLuaState = std::make_shared<LuaUIState>();
                        if (previewLuaState->LoadStateFile(tempLuaFile))
                        {
                            processedHTML = parser.Evaluate(*previewLuaState);
                        }

                        std::filesystem::remove(tempLuaFile);
                    }
                }
            }
            catch (const std::exception &e)
            {
                LOG_WARNING("[Editor] Failed to process template directives in preview: {}", e.what());
                // Fall through to show HTML+CSS without directive processing
            }
        }

        // Update preview HTML
        luaState->SetValue("uiEditor.previewHTML", processedHTML);
        luaState->SetValue("uiEditor.errorMessage", "");
        luaState->MarkDirty();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("[Editor] Exception while updating preview: {}", e.what());
        luaState->SetValue("uiEditor.errorMessage", "Preview error: " + std::string(e.what()));
        luaState->SetValue("uiEditor.previewHTML", "<p style='color: #f48771;'>Error: " + std::string(e.what()) + "</p>");
        luaState->MarkDirty();
    }
}

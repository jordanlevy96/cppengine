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
#include "controllers/Game.h"
#include "controllers/ScriptManager.h"

#include <sol/sol.hpp>
#include <glm/gtc/quaternion.hpp>
#include <fstream>
#include <iostream>
#include <variant>

bool Editor::Initialize()
{
    std::cout << "[Editor] Starting initialization..." << std::endl;

    if (m_initialized)
    {
        std::cerr << "[Editor] Already initialized" << std::endl;
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

    // Load editor UI
    LoadEditorUI();

    // Populate scene tree with entities from Registry
    UpdateSceneTree();

    // Register keyboard input handler for editor shortcuts
    m_keyboardHandlerId = m_windowManager->RegisterInputHandler([this](const InputEvent &event)
                                                                { return HandleEditorInput(event); });

    // TODO: clean up this lambda
    // Register click input handler for UI interactions
    m_clickHandlerId = m_windowManager->RegisterInputHandler([this](const InputEvent &event)
                                                             {
                                                                 if (event.type == InputTypes::Click)
                                                                 {
                                                                     // Safely extract click payload - some platforms store vec2, some vec3
                                                                     try
                                                                     {
                                                                         if (std::holds_alternative<glm::vec3>(event.input))
                                                                         {
                                                                             auto clickData = std::get<glm::vec3>(event.input);
                                                                             LOG_INFO("[Editor] Click handler: click at ({}, {}), button={}", clickData.x, clickData.y, static_cast<int>(clickData.z));
                                                                             bool handled = HandleUIClick(clickData.x, clickData.y, static_cast<int>(clickData.z));
                                                                             LOG_INFO("[Editor] Click handler result: {}", handled ? "CONSUMED" : "PASS_TO_LUA");
                                                                             return handled;
                                                                         }
                                                                         else if (std::holds_alternative<glm::vec2>(event.input))
                                                                         {
                                                                             auto clickData2 = std::get<glm::vec2>(event.input);
                                                                             // If button isn't encoded, assume left (0)
                                                                             LOG_INFO("[Editor] Click handler: click at ({}, {}), button=0", clickData2.x, clickData2.y);
                                                                             bool handled = HandleUIClick(clickData2.x, clickData2.y, 0);
                                                                             LOG_INFO("[Editor] Click handler result: {}", handled ? "CONSUMED" : "PASS_TO_LUA");
                                                                             return handled;
                                                                         }
                                                                         else
                                                                         {
                                                                             LOG_WARNING("[Editor] Click event payload has unexpected type");
                                                                             return false;
                                                                         }
                                                                     }
                                                                     catch (const std::bad_variant_access &)
                                                                     {
                                                                         LOG_WARNING("[Editor] Failed to read click event payload");
                                                                         return false;
                                                                     }
                                                                 }
                                                                 return false; // Not a click event
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
        LOG_INFO("[Editor::methods.selectEntity] Called with entityId={}", entityId);
        SelectEntity(static_cast<EntityID>(entityId));
    };

    luaUIState->SetValue("methods", methods);
    LOG_INFO("[Editor] Created methods table for UI event handlers");

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

    static bool loggedDataURI = false;
    if (m_reactiveUI && m_reactiveUI->GetLuaState())
    {
        if (!viewportDataURI.empty())
        {
            if (!loggedDataURI)
            {
                LOG_INFO("Setting viewportImage in Lua state: {} bytes", viewportDataURI.size());
                // Log first 100 chars to verify it's a valid data URI
                std::string preview = viewportDataURI.substr(0, std::min<size_t>(100, viewportDataURI.size()));
                LOG_INFO("Data URI preview: {}", preview);
                loggedDataURI = true;
            }

            m_reactiveUI->GetLuaState()->SetValue("viewportImage", viewportDataURI);
            m_reactiveUI->GetLuaState()->MarkDirty(); // Trigger UI re-render
        }
        else
        {
            static bool warned = false;
            if (!warned)
            {
                LOG_WARNING("Viewport data URI is empty - image conversion may have failed");
                warned = true;
            }
        }
    }

    // Update HTML if Lua state changed (GetRenderedHTML re-renders if dirty)
    static bool loggedHTMLUpdate = false;
    if (m_reactiveUI)
    {
        std::string renderedHTML = m_reactiveUI->GetRenderedHTML();

        if (!loggedHTMLUpdate)
        {
            LOG_INFO("Rendered HTML size: {} bytes", renderedHTML.size());
            // Check if viewportImage is in the rendered HTML
            if (renderedHTML.find("data:image/png;base64") != std::string::npos)
            {
                LOG_INFO("✓ Viewport image data URI found in rendered HTML");
            }
            else if (renderedHTML.find("viewportImage") != std::string::npos)
            {
                LOG_WARNING("Template variable {{ viewportImage }} not replaced in HTML");
            }
            else
            {
                LOG_WARNING("No viewport image reference found in rendered HTML");
            }
            loggedHTMLUpdate = true;
        }

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
    LOG_INFO("Loading editor UI...");

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

    LOG_INFO("Loaded editor HTML template: {} bytes", editorHTML.size());

    // Register template with ReactiveUI so directives (v-for, v-if, etc.) are processed
    m_reactiveUI->RegisterTemplateWithDirectives("EditorUI", editorHTML);

    // Get the rendered HTML and send to HTMLRenderer
    std::string renderedHTML = m_reactiveUI->GetRenderedHTML();
    m_htmlRenderer->LoadHTML(renderedHTML);

    LOG_INFO("Editor UI loaded, rendered HTML: {} bytes", renderedHTML.size());
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

    LOG_INFO("Scene tree updated with {} entities", entityIds.size());
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
    if (entityId == ENTITY_NULL)
    {
        m_selectedEntityId = ENTITY_NULL;
        LOG_DEBUG("Deselected entity");
    }
    else
    {
        m_selectedEntityId = entityId;
        LOG_DEBUG("Selected entity: {}", m_registry->GetEntityName(entityId));
    }

    // Update viewport highlight
    if (m_viewport)
    {
        m_viewport->SetSelectedEntity(entityId);
    }

    UpdateInspector();

    // Mark Lua state dirty to trigger UI re-render
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
    LOG_DEBUG("[Editor::HandleUIClick] Called with position ({}, {}), button={}", x, y, button);

    if (!m_htmlRenderer)
    {
        LOG_WARNING("[Editor::HandleUIClick] HTMLRenderer not initialized");
        return false;
    }

    LOG_DEBUG("[Editor::HandleUIClick] Calling HTMLRenderer::HandleClickEvent");
    // Pass click to HTMLRenderer for hit-testing and event dispatch
    // This will check if the click hit any UI elements and dispatch @click events
    return m_htmlRenderer->HandleClickEvent(x, y, button);
}

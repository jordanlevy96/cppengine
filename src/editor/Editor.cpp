/**
 * @file Editor.cpp
 * @brief Editor implementation - Phase 2
 */

#include "editor/Editor.h"
#include "util/Logger.h"
#include "util/ConfigLoader.h"
#include "systems/LuaUIState.h"
#include "systems/ScriptSystem.h"
#include "systems/TweenSystem.h"
#include "controllers/Game.h"
#include "controllers/ScriptManager.h"

#include <sol/sol.hpp>
#include <fstream>
#include <iostream>

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

    // Register keyboard shortcuts handler
    m_keyboardHandlerId = m_windowManager->RegisterInputHandler([this](const InputEvent &event)
                                                                {
                                                                    if (event.type == InputTypes::Key)
                                                                    {
                                                                        // ESC to quit editor
                                                                        if (std::get<std::string>(event.input) == "ESCAPE")
                                                                        {
                                                                            LOG_INFO("ESC pressed - closing editor");
                                                                            m_shouldClose = true;
                                                                            return true;
                                                                        }
                                                                    }
                                                                    return false; // Don't consume other events
                                                                });

    // Initialize timing
    m_lastFrameTime = std::chrono::high_resolution_clock::now();
    m_delta = 0.0;

    m_initialized = true;
    LOG_INFO("Editor initialized successfully");
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

    while (!glfwWindowShouldClose(m_windowManager->window) && !m_shouldClose)
    {
        // Calculate delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        m_delta = std::chrono::duration_cast<std::chrono::milliseconds>(
                      currentTime - m_lastFrameTime)
                      .count();
        m_lastFrameTime = currentTime;

        // Poll input events
        glfwPollEvents();

        // Update game systems
        UpdateSystems(m_delta);

        // Render frame
        Render();
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
    m_windowManager->UnregisterInputHandler(m_keyboardHandlerId);

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

    // Update tween system (animations)
    TweenSystem::Update(deltaTime);
}

/**
 * @file Editor.cpp
 * @brief Editor implementation - Phase 1
 */

#include "editor/Editor.h"
#include "util/Logger.h"

#include <iostream>

bool Editor::Initialize()
{
    if (m_initialized)
    {
        LOG_WARNING("Editor already initialized");
        return true;
    }

    LOG_INFO("Initializing Imhotep Editor...");

    // Get singleton references
    m_windowManager = &WindowManager::GetInstance();
    m_registry = &Registry::GetInstance();
    m_htmlRenderer = &HTMLRendererMT::GetInstance();
    m_reactiveUI = &ReactiveUI::GetInstance();

    // Initialize window (1920x1080 for editor)
    if (!m_windowManager->Initialize(1920, 1080))
    {
        LOG_CRITICAL("Failed to initialize WindowManager");
        return false;
    }

    // Get framebuffer size (handles Retina/HiDPI scaling)
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_windowManager->window, &fbWidth, &fbHeight);
    LOG_INFO("Framebuffer size: {}x{}", fbWidth, fbHeight);

    // Initialize HTML renderer with framebuffer size
    m_htmlRenderer->Initialize(m_windowManager->window, fbWidth, fbHeight);

    // Load editor UI
    LoadEditorUI();

    // Register keyboard shortcuts handler
    m_keyboardHandlerId = m_windowManager->RegisterInputHandler([this](const InputEvent& event) {
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
        return false;  // Don't consume other events
    });

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
        glfwPollEvents();
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
    m_htmlRenderer->Shutdown();
    m_windowManager->Shutdown();

    m_initialized = false;
    LOG_INFO("Editor shutdown complete");
}

void Editor::Render()
{
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

    // Phase 1: Simple placeholder HTML
    // TODO: Load from res/editor/templates/ in later phases
    std::string editorHTML = R"(
<!DOCTYPE html>
<html>
<head>
    <style>
        body {
            margin: 0;
            padding: 0;
            font-family: -apple-system, system-ui, sans-serif;
            background: #1e1e1e;
            color: #cccccc;
        }
        .editor-container {
            display: flex;
            flex-direction: column;
            height: 100vh;
        }
        .menu-bar {
            background: #252526;
            padding: 8px 16px;
            border-bottom: 1px solid #3e3e42;
            font-size: 13px;
        }
        .menu-bar span {
            margin-right: 20px;
            cursor: pointer;
        }
        .menu-bar span:hover {
            color: #ffffff;
        }
        .content {
            display: flex;
            flex: 1;
            overflow: hidden;
        }
        .panel {
            background: #1e1e1e;
            border: 1px solid #3e3e42;
            margin: 4px;
            padding: 16px;
        }
        .scene-tree {
            width: 250px;
        }
        .viewport {
            flex: 1;
            display: flex;
            align-items: center;
            justify-content: center;
            background: #2d2d30;
        }
        .inspector {
            width: 300px;
        }
        h3 {
            margin: 0 0 16px 0;
            font-size: 14px;
            font-weight: 600;
            text-transform: uppercase;
            color: #d4d4d4;
        }
        .placeholder {
            color: #d4d4d4;
            font-size: 14px;
        }
        .welcome {
            text-align: center;
            padding: 40px;
        }
        .welcome h1 {
            font-size: 32px;
            font-weight: 300;
            margin-bottom: 16px;
            color: #ffffff;
        }
        .welcome p {
            font-size: 14px;
            color: #d4d4d4;
        }
    </style>
</head>
<body>
    <div class="editor-container">
        <div class="menu-bar">
            <span>File</span>
            <span>Edit</span>
            <span>View</span>
            <span>Tools</span>
        </div>
        <div class="content">
            <div class="panel scene-tree">
                <h3>Scene Hierarchy</h3>
                <div class="placeholder">No scene loaded</div>
            </div>
            <div class="panel viewport">
                <div class="welcome">
                    <h1>Imhotep Editor</h1>
                    <p>Phase 1: Foundation</p>
                    <p style="margin-top: 24px; font-size: 12px;">Press ESC to quit</p>
                </div>
            </div>
            <div class="panel inspector">
                <h3>Inspector</h3>
                <div class="placeholder">No entity selected</div>
            </div>
        </div>
    </div>
</body>
</html>
    )";

    m_htmlRenderer->LoadHTML(editorHTML);
    LOG_INFO("Editor UI loaded");
}

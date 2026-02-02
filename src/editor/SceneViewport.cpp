/**
 * @file SceneViewport.cpp
 * @brief SceneViewport implementation
 */

#include "editor/SceneViewport.h"
#include "systems/RenderSystem.h"
#include "components/RenderComponent.h"
#include "components/Lighting.h"
#include "components/WorldTransform.h"
#include "util/Shader.h"
#include "util/Logger.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

SceneViewport::~SceneViewport()
{
    Shutdown();
}

bool SceneViewport::Initialize(int width, int height, Camera *camera)
{
    if (m_initialized)
    {
        LOG_WARNING("SceneViewport already initialized");
        return true;
    }

    if (!camera)
    {
        LOG_ERROR("SceneViewport: camera pointer is null");
        return false;
    }

    LOG_INFO("Initializing SceneViewport: {}x{}", width, height);

    m_width = width;
    m_height = height;
    m_camera = camera;

    // Get Registry reference
    m_registry = &Registry::GetInstance();

    // Update camera perspective for viewport dimensions
    m_camera->SetPerspective(45.0f, static_cast<float>(width), static_cast<float>(height));
    m_camera->transform.Pos = glm::vec3(0.0f, 5.0f, 15.0f); // Position behind and above origin

    // Create framebuffer
    if (!CreatePickFramebuffer())
    {
        LOG_ERROR("Failed to create framebuffer");
        return false;
    }

    // Create picking shader
    if (!m_pickShader)
    {
        m_pickShader = new Shader("../res/shaders/Picking.shader");
    }

    m_initialized = true;
    LOG_INFO("SceneViewport initialized successfully");
    return true;
}

bool SceneViewport::CreatePickFramebuffer()
{
    // Create picking texture (RGBA8) + framebuffer
    glGenTextures(1, &m_pickTexture);
    glBindTexture(GL_TEXTURE_2D, m_pickTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffers(1, &m_pickDepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_pickDepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_width, m_height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &m_pickFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_pickFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pickTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_pickDepthBuffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    GLenum pickStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (pickStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG_ERROR("Pick framebuffer not complete: 0x{:x}", pickStatus);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        DeleteFramebuffer();
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

void SceneViewport::DeleteFramebuffer()
{
    if (m_pickFramebuffer != 0)
    {
        glDeleteFramebuffers(1, &m_pickFramebuffer);
        m_pickFramebuffer = 0;
    }
    if (m_pickTexture != 0)
    {
        glDeleteTextures(1, &m_pickTexture);
        m_pickTexture = 0;
    }
    if (m_pickDepthBuffer != 0)
    {
        glDeleteRenderbuffers(1, &m_pickDepthBuffer);
        m_pickDepthBuffer = 0;
    }
}

void SceneViewport::RenderToScreen(int fbX, int fbY, int fbWidth, int fbHeight)
{
    if (!m_initialized)
    {
        LOG_ERROR("Cannot render - SceneViewport not initialized");
        return;
    }

    // Render only within the viewport rectangle.
    glEnable(GL_SCISSOR_TEST);
    glViewport(fbX, fbY, fbWidth, fbHeight);
    glScissor(fbX, fbY, fbWidth, fbHeight);

    // Clear with dark gray background (inside viewport only)
    glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    // Enable depth testing for 3D rendering
    glEnable(GL_DEPTH_TEST);

    // Debug: Log entity counts
    static bool logged = false;
    if (!logged)
    {
        size_t renderCount = m_registry->GetComponentSet<RenderComponent>().GetEntities().size();
        size_t lightingCount = m_registry->GetComponentSet<Lighting>().GetEntities().size();
        LOG_INFO("SceneViewport rendering: {} RenderComponents, {} Lighting components", renderCount, lightingCount);
        LOG_INFO("Camera position: ({}, {}, {})", m_camera->transform.Pos.x, m_camera->transform.Pos.y, m_camera->transform.Pos.z);
        logged = true;
    }

    // Render scene using RenderSystem
    RenderSystem::Update(m_camera, 0.0f);

    // Render selection highlight (if entity is selected)
    if (m_selectedEntityId != ENTITY_NULL)
    {
        // Check if entity has required components for rendering
        if (m_registry->HasComponent<Transform>(m_selectedEntityId) &&
            m_registry->HasComponent<RenderComponent>(m_selectedEntityId))
        {
            Transform &transform = m_registry->GetComponent<Transform>(m_selectedEntityId);
            RenderComponent &rc = m_registry->GetComponent<RenderComponent>(m_selectedEntityId);

            // Save original color
            glm::vec4 originalColor = transform.Color;

            // Set bright highlight color (yellow/orange with full opacity)
            transform.Color = glm::vec4(1.0f, 0.8f, 0.0f, 1.0f);

            // Enable wireframe mode for outline effect
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            // Note: glLineWidth(3.0f) causes GL_INVALID_VALUE on macOS/Metal
            // Only 1.0f is supported - using default width

            // Disable depth test so outline draws on top
            glDisable(GL_DEPTH_TEST);

            // Render entity again as wireframe
            RenderSystem::RenderEntity<RenderComponent>(m_selectedEntityId, m_camera);

            // Restore OpenGL state
            glEnable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            // Restore original color
            transform.Color = originalColor;
        }
    }
}

void SceneViewport::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    LOG_INFO("Shutting down SceneViewport");
    if (m_pickShader)
    {
        delete m_pickShader;
        m_pickShader = nullptr;
    }
    DeleteFramebuffer();
    m_initialized = false;
}

void SceneViewport::Resize(int width, int height)
{
    if (width == m_width && height == m_height)
    {
        return;
    }

    LOG_DEBUG("Resizing SceneViewport: {}x{} -> {}x{}", m_width, m_height, width, height);

    m_width = width;
    m_height = height;

    // Update camera perspective
    m_camera->SetPerspective(m_camera->fov, static_cast<float>(width), static_cast<float>(height));

    // Recreate pick framebuffer
    DeleteFramebuffer();
    CreatePickFramebuffer();
}

EntityID SceneViewport::PickEntityAt(int x, int y)
{
    if (!m_initialized || m_pickFramebuffer == 0)
    {
        return ENTITY_NULL;
    }

    if (x < 0 || y < 0 || x >= m_width || y >= m_height)
    {
        return ENTITY_NULL;
    }

    // Convert from top-left origin (UI) to bottom-left origin (OpenGL readback)
    int readY = (m_height - 1) - y;

    // Render a fresh ID pass before reading.
    RenderPickingPass();

    uint8_t pixel[4] = {0, 0, 0, 0};
    glBindFramebuffer(GL_FRAMEBUFFER, m_pickFramebuffer);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(x, readY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel[0]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);

    uint32_t id24 = static_cast<uint32_t>(pixel[0]) |
                    (static_cast<uint32_t>(pixel[1]) << 8) |
                    (static_cast<uint32_t>(pixel[2]) << 16);

    if (id24 == 0)
    {
        return ENTITY_NULL;
    }

    return static_cast<EntityID>(id24 - 1u);
}

void SceneViewport::RenderPickingPass()
{
    if (!m_initialized || m_pickFramebuffer == 0 || !m_pickShader)
    {
        return;
    }

    GLint prevDrawFbo = 0;
    GLint prevReadFbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFbo);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFbo);

    GLint prevViewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    GLint prevScissorBox[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_SCISSOR_BOX, prevScissorBox);

    GLint prevPolygonMode[2] = {GL_FILL, GL_FILL};
    glGetIntegerv(GL_POLYGON_MODE, prevPolygonMode);

    GLint prevProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);

    GLint prevVao = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);

    GLboolean prevBlendEnabled = glIsEnabled(GL_BLEND);
    GLboolean prevDepthEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevScissorEnabled = glIsEnabled(GL_SCISSOR_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, m_pickFramebuffer);
    glViewport(0, 0, m_width, m_height);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_pickShader->Use();

    glm::vec3 cameraPos = m_camera->transform.Pos;
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + m_camera->front, m_camera->up);
    m_pickShader->SetMat4("view", view);
    m_pickShader->SetMat4("projection", m_camera->Projection);

    const auto &renderEntities = m_registry->GetComponentSet<RenderComponent>().GetEntities();
    for (EntityID id : renderEntities)
    {
        if (!m_registry->HasComponent<WorldTransform>(id) || !m_registry->HasComponent<RenderComponent>(id))
        {
            continue;
        }

        WorldTransform &wt = m_registry->GetComponent<WorldTransform>(id);
        RenderComponent &rc = m_registry->GetComponent<RenderComponent>(id);
        if (!rc.mesh)
        {
            continue;
        }

        // Encode EntityID into 24-bit RGB with +1 offset so "0" remains reserved for "no entity".
        // This allows entityId==0 to be pickable and avoids ambiguity with the clear color.
        uint32_t id32 = static_cast<uint32_t>(id);
        if (id32 >= 0x00FFFFFEu)
        {
            continue; // Out of 24-bit range for picking.
        }

        uint32_t encoded = (id32 + 1u) & 0x00FFFFFFu;
        float r = static_cast<float>((encoded >> 0) & 0xFF) / 255.0f;
        float g = static_cast<float>((encoded >> 8) & 0xFF) / 255.0f;
        float b = static_cast<float>((encoded >> 16) & 0xFF) / 255.0f;

        m_pickShader->SetMat4("model", wt.matrix);
        m_pickShader->SetVec4("idColor", r, g, b, 1.0f);

        glBindVertexArray(rc.mesh->VAO);
        glDrawElements(GL_TRIANGLES, rc.mesh->indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // Restore GL state (picking is invoked during input handling, outside normal render flow).
    if (prevBlendEnabled)
    {
        glEnable(GL_BLEND);
    }
    else
    {
        glDisable(GL_BLEND);
    }

    if (prevDepthEnabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    if (prevScissorEnabled)
    {
        glEnable(GL_SCISSOR_TEST);
        glScissor(prevScissorBox[0], prevScissorBox[1], prevScissorBox[2], prevScissorBox[3]);
    }
    else
    {
        glDisable(GL_SCISSOR_TEST);
    }

    glPolygonMode(GL_FRONT, prevPolygonMode[0]);
    glPolygonMode(GL_BACK, prevPolygonMode[1]);

    glUseProgram(static_cast<GLuint>(prevProgram));
    glBindVertexArray(static_cast<GLuint>(prevVao));

    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFbo);
}

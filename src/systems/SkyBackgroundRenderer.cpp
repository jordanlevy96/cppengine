/**
 * @file SkyBackgroundRenderer.cpp
 * @brief Fullscreen sky gradient + sun background pass
 * @lines ~240
 *
 * Purpose:
 * - Draws a fullscreen sky gradient behind the scene (Milestone 2).
 * - Keeps sky rendering decoupled from the tiled ground renderer.
 *
 * Key functions:
 * - Initialize() - Load shader and create VAO for fullscreen triangle (line ~45, ~70 lines)
 * - Render() - Configure uniforms and draw with depth writes disabled (line ~135, ~70 lines)
 */

#include "systems/SkyBackgroundRenderer.h"

#include "controllers/Game.h"
#include "util/Logger.h"
#include "util/Shader.h"

#include <glad/glad.h>

#include <algorithm>

SkyBackgroundRenderer::~SkyBackgroundRenderer() = default;

bool SkyBackgroundRenderer::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    // Require OpenGL context.
    glGetString(GL_VERSION);

    const std::string &res = Game::GetInstance().conf.ResourcePath;
    const std::string shaderPath = res + "shaders/SkyBackground.shader";

    try
    {
        m_shader = std::make_unique<Shader>(shaderPath);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("SkyBackgroundRenderer: failed to load shader {} ({})", shaderPath, e.what());
        return false;
    }

    EnsureDrawResources();
    m_initialized = true;
    LOG_INFO("SkyBackgroundRenderer initialized");
    return true;
}

bool SkyBackgroundRenderer::EnsureInitialized()
{
    if (!m_initialized)
    {
        return Initialize();
    }
    return true;
}

void SkyBackgroundRenderer::Shutdown()
{
    if (m_vao != 0)
    {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }

    m_shader.reset();
    m_initialized = false;
}

void SkyBackgroundRenderer::Configure(const SkyBackgroundConfig &config)
{
    m_config = config;

    m_config.horizonY = std::clamp(m_config.horizonY, 0.0f, 1.0f);
    m_config.horizonGlow = std::max(0.0f, m_config.horizonGlow);
    m_config.sunRadius = std::max(0.0f, m_config.sunRadius);
    m_config.sunGlow = std::max(0.0f, m_config.sunGlow);

    EnsureInitialized();
}

void SkyBackgroundRenderer::SetEnabled(bool enabled)
{
    m_config.enabled = enabled;
    if (enabled)
    {
        EnsureInitialized();
    }
}

void SkyBackgroundRenderer::EnsureDrawResources()
{
    if (m_vao != 0)
    {
        return;
    }

    // Fullscreen triangle (no VBO): positions are generated in the vertex shader using gl_VertexID.
    glGenVertexArrays(1, &m_vao);
}

void SkyBackgroundRenderer::DrawFullscreen() const
{
    if (m_vao == 0 || !m_shader)
    {
        return;
    }

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void SkyBackgroundRenderer::Render(Camera *camera, float deltaMs)
{
    (void)camera;
    (void)deltaMs;

    if (!m_config.enabled)
    {
        return;
    }

    if (!EnsureInitialized())
    {
        return;
    }

    const GLboolean wasDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean wasDepthMask = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &wasDepthMask);
    const GLboolean wasCullEnabled = glIsEnabled(GL_CULL_FACE);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    m_shader->Use();
    m_shader->SetVec3("u_topColor", m_config.topColor);
    m_shader->SetVec3("u_bottomColor", m_config.bottomColor);
    m_shader->SetFloat("u_horizonY", m_config.horizonY);
    m_shader->SetFloat("u_horizonGlow", m_config.horizonGlow);
    m_shader->SetVec3("u_horizonColor", m_config.horizonColor);
    m_shader->SetVec2("u_sunPos", m_config.sunPos);
    m_shader->SetFloat("u_sunRadius", m_config.sunRadius);
    m_shader->SetFloat("u_sunGlow", m_config.sunGlow);
    m_shader->SetVec3("u_sunColor", m_config.sunColor);

    DrawFullscreen();

    glUseProgram(0);

    if (wasDepthTestEnabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(wasDepthMask);
    if (wasCullEnabled)
    {
        glEnable(GL_CULL_FACE);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
}


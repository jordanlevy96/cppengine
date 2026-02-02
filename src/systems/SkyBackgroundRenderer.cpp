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

    m_config.mountainBaseY = std::clamp(m_config.mountainBaseY, 0.0f, 1.0f);
    m_config.mountainHeight = std::max(0.0f, m_config.mountainHeight);
    m_config.mountainScale = std::max(0.0f, m_config.mountainScale);
    m_config.mountainDetail = std::clamp(m_config.mountainDetail, 0.0f, 1.0f);
    m_config.mountainScrollSpeed = std::max(0.0f, m_config.mountainScrollSpeed);
    m_config.mountainEdgePixels = std::max(0.0f, m_config.mountainEdgePixels);

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
    m_timeSeconds += std::max(0.0f, deltaMs) * 0.001f;

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

    GLint vp[4] = {0, 0, 1, 1};
    glGetIntegerv(GL_VIEWPORT, vp);
    const float aspect = (vp[3] > 0) ? (static_cast<float>(vp[2]) / static_cast<float>(vp[3])) : 1.0f;

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
    m_shader->SetFloat("u_time", m_timeSeconds);
    m_shader->SetFloat("u_aspect", aspect);

    m_shader->SetInt("u_mountainsEnabled", m_config.mountainsEnabled ? 1 : 0);
    m_shader->SetInt("u_mountainsOccludeSun", m_config.mountainsOccludeSun ? 1 : 0);
    m_shader->SetVec3("u_mountainColor", m_config.mountainColor);
    m_shader->SetFloat("u_mountainBaseY", m_config.mountainBaseY);
    m_shader->SetFloat("u_mountainHeight", m_config.mountainHeight);
    m_shader->SetFloat("u_mountainScale", m_config.mountainScale);
    m_shader->SetFloat("u_mountainDetail", m_config.mountainDetail);
    m_shader->SetFloat("u_mountainScrollSpeed", m_config.mountainScrollSpeed);
    m_shader->SetFloat("u_mountainEdgePixels", m_config.mountainEdgePixels);

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

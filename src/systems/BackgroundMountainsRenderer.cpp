/**
 * @file BackgroundMountainsRenderer.cpp
 * @brief Procedural mountain mesh with wire grid overlay (background terrain)
 * @lines ~280
 *
 * Purpose:
 * - Draws a camera-oriented mountain ridge behind the ground grid.
 * - Uses a procedural height function in the shader, so future "dynamic mountains" are possible.
 *
 * Key functions:
 * - Initialize() - Load shader and create mesh buffers (line ~40, ~90 lines)
 * - Render() - Set uniforms and draw with depth writes disabled (line ~160, ~90 lines)
 */

#include "systems/BackgroundMountainsRenderer.h"

#include "controllers/Game.h"
#include "util/Logger.h"
#include "util/Shader.h"

#include <glad/glad.h>

#include <algorithm>
#include <vector>

BackgroundMountainsRenderer::~BackgroundMountainsRenderer() = default;

bool BackgroundMountainsRenderer::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    // Require OpenGL context.
    glGetString(GL_VERSION);

    const std::string &res = Game::GetInstance().conf.ResourcePath;
    const std::string shaderPath = res + "shaders/BackgroundMountains.shader";

    try
    {
        m_shader = std::make_unique<Shader>(shaderPath);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("BackgroundMountainsRenderer: failed to load shader {} ({})", shaderPath, e.what());
        return false;
    }

    EnsureMeshResources();
    m_initialized = true;
    LOG_INFO("BackgroundMountainsRenderer initialized");
    return true;
}

bool BackgroundMountainsRenderer::EnsureInitialized()
{
    if (!m_initialized)
    {
        return Initialize();
    }
    return true;
}

void BackgroundMountainsRenderer::Shutdown()
{
    if (m_vbo != 0)
    {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao != 0)
    {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }

    m_shader.reset();
    m_initialized = false;
}

void BackgroundMountainsRenderer::Configure(const BackgroundMountainsConfig &config)
{
    m_config = config;
    m_config.startDistance = std::max(0.0f, m_config.startDistance);
    m_config.depth = std::max(0.0f, m_config.depth);
    m_config.widthMultiplier = std::max(0.1f, m_config.widthMultiplier);
    m_config.height = std::max(0.0f, m_config.height);
    m_config.noiseScale = std::max(0.0f, m_config.noiseScale);
    m_config.detail = std::clamp(m_config.detail, 0.0f, 1.0f);
    m_config.scrollSpeed = std::max(0.0f, m_config.scrollSpeed);
    m_config.gridSpacing = std::max(0.0001f, m_config.gridSpacing);
    m_config.majorEvery = std::max(1, m_config.majorEvery);
    m_config.minorLineWidth = std::max(0.0f, m_config.minorLineWidth);
    m_config.majorLineWidth = std::max(0.0f, m_config.majorLineWidth);

    EnsureInitialized();
}

void BackgroundMountainsRenderer::SetEnabled(bool enabled)
{
    m_config.enabled = enabled;
    if (enabled)
    {
        EnsureInitialized();
    }
}

void BackgroundMountainsRenderer::EnsureMeshResources()
{
    if (m_vao != 0)
    {
        return;
    }

    // Grid in (u,v) for a heightfield strip.
    // u: [0..1] across width, v: [0..1] along depth.
    const int uSeg = 160;
    const int vSeg = 48;

    std::vector<float> verts;
    verts.reserve(static_cast<size_t>(uSeg * vSeg * 6 * 2));

    auto push = [&](float u, float v)
    {
        verts.push_back(u);
        verts.push_back(v);
    };

    for (int y = 0; y < vSeg; y++)
    {
        const float v0 = static_cast<float>(y) / static_cast<float>(vSeg);
        const float v1 = static_cast<float>(y + 1) / static_cast<float>(vSeg);
        for (int x = 0; x < uSeg; x++)
        {
            const float u0 = static_cast<float>(x) / static_cast<float>(uSeg);
            const float u1 = static_cast<float>(x + 1) / static_cast<float>(uSeg);

            // Tri 1
            push(u0, v0);
            push(u1, v0);
            push(u1, v1);
            // Tri 2
            push(u0, v0);
            push(u1, v1);
            push(u0, v1);
        }
    }

    m_vertexCount = static_cast<int>(verts.size() / 2);

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * verts.size(), verts.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void BackgroundMountainsRenderer::Draw(Camera *camera) const
{
    if (!m_shader || m_vao == 0 || m_vertexCount <= 0 || !camera)
    {
        return;
    }

    const glm::vec3 cameraPos = camera->transform.Pos;
    glm::vec3 forward = camera->front;
    forward.y = 0.0f;
    const float fLen = glm::length(forward);
    if (fLen < 0.0001f)
    {
        forward = glm::vec3(0.0f, 0.0f, -1.0f);
    }
    else
    {
        forward /= fLen;
    }
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

    const float aspect = camera->Projection[1][1] != 0.0f ? (camera->Projection[1][1] / camera->Projection[0][0]) : 1.0f;
    const float tanHalfFov = std::tan(glm::radians(camera->fov) * 0.5f);
    const float halfWidth = (m_config.startDistance + m_config.depth * 0.25f) * tanHalfFov * aspect * m_config.widthMultiplier;

    m_shader->Use();
    m_shader->SetMat4("view", glm::lookAt(cameraPos, cameraPos + camera->front, camera->up));
    m_shader->SetMat4("projection", camera->Projection);

    m_shader->SetVec2("u_cameraPosXZ", glm::vec2(cameraPos.x, cameraPos.z));
    m_shader->SetVec2("u_cameraForwardXZ", glm::vec2(forward.x, forward.z));
    m_shader->SetVec2("u_cameraRightXZ", glm::vec2(right.x, right.z));

    m_shader->SetFloat("u_startDistance", m_config.startDistance);
    m_shader->SetFloat("u_depth", m_config.depth);
    m_shader->SetFloat("u_halfWidth", halfWidth);

    m_shader->SetFloat("u_baseY", m_config.baseY);
    m_shader->SetFloat("u_height", m_config.height);
    m_shader->SetFloat("u_noiseScale", m_config.noiseScale);
    m_shader->SetFloat("u_detail", m_config.detail);
    m_shader->SetFloat("u_scrollSpeed", m_config.scrollSpeed);
    m_shader->SetFloat("u_time", m_timeSeconds);

    m_shader->SetVec4("u_fillColor", m_config.fillColor);
    m_shader->SetFloat("u_gridSpacing", m_config.gridSpacing);
    m_shader->SetInt("u_majorEvery", m_config.majorEvery);
    m_shader->SetFloat("u_minorLineWidth", m_config.minorLineWidth);
    m_shader->SetFloat("u_majorLineWidth", m_config.majorLineWidth);
    m_shader->SetVec4("u_minorLineColor", m_config.minorLineColor);
    m_shader->SetVec4("u_majorLineColor", m_config.majorLineColor);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
    glBindVertexArray(0);
    glUseProgram(0);
}

void BackgroundMountainsRenderer::Render(Camera *camera, float deltaMs)
{
    m_timeSeconds += std::max(0.0f, deltaMs) * 0.001f;

    if (!m_config.enabled)
    {
        return;
    }
    if (!camera)
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

    Draw(camera);

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


/**
 * @file SplashScreen.cpp
 * @brief Static splash screen rendered during engine boot
 * @lines ~120
 *
 * Purpose: Displays a fullscreen PNG texture while subsystems initialize.
 * Uses the same Composite.shader and fullscreen quad pattern as HTMLRendererMT.
 *
 * Key functions:
 * - Initialize() - Load PNG via stb_image, create GL texture + quad (line ~25, ~60 lines)
 * - Render() - Clear screen, draw textured quad (line ~90, ~25 lines)
 * - Shutdown() - Delete GL resources (line ~115, ~10 lines)
 */

#include "systems/SplashScreen.h"
#include "controllers/Game.h"
#include "util/Shader.h"
#include "util/Logger.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

bool SplashScreen::Initialize(const std::string &pngPath, int viewportWidth, int viewportHeight)
{
    m_width = viewportWidth;
    m_height = viewportHeight;

    // Load PNG (top-left origin to match Composite shader expectations)
    stbi_set_flip_vertically_on_load(0);

    int imgWidth, imgHeight, channels;
    unsigned char *pixels = stbi_load(pngPath.c_str(), &imgWidth, &imgHeight, &channels, 4);

    // Restore default flip state
    stbi_set_flip_vertically_on_load(1);

    if (!pixels)
    {
        LOG_WARNING("[SplashScreen] Failed to load splash PNG: {}", pngPath);
        return false;
    }

    LOG_INFO("[SplashScreen] Loaded splash PNG: {}x{} ({}ch)", imgWidth, imgHeight, channels);

    // Create GL texture
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);

    // Load Composite shader (reuses existing shader file)
    Shader *shader = new Shader(Game::GetInstance().conf.ResourcePath + "shaders/Composite.shader");
    m_shaderProgram = shader->ID;
    // Prevent Shader destructor from deleting the GL program
    shader->ID = 0;
    delete shader;

    // Create fullscreen quad (same layout as HTMLRendererMT)
    float quadVertices[] = {
        // positions                              // texCoords
        0.0f,            0.0f,            0.0f, 0.0f,  // top-left
        0.0f,            (float)m_height, 0.0f, 1.0f,  // bottom-left
        (float)m_width,  (float)m_height, 1.0f, 1.0f,  // bottom-right

        0.0f,            0.0f,            0.0f, 0.0f,  // top-left
        (float)m_width,  (float)m_height, 1.0f, 1.0f,  // bottom-right
        (float)m_width,  0.0f,            1.0f, 0.0f,  // top-right
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    m_ready = true;
    LOG_INFO("[SplashScreen] Initialized ({}x{})", m_width, m_height);
    return true;
}

void SplashScreen::Render()
{
    if (!m_ready)
        return;

    glViewport(0, 0, m_width, m_height);
    glClearColor(0.08f, 0.07f, 0.06f, 1.0f);  // Dark charcoal matching splash background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(m_shaderProgram);

    // Set ortho projection (top-left origin)
    glm::mat4 projection = glm::ortho(0.0f, (float)m_width, (float)m_height, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "screenTexture"), 0);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

void SplashScreen::Shutdown()
{
    if (m_texture)
    {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }
    if (m_quadVAO)
    {
        glDeleteVertexArrays(1, &m_quadVAO);
        m_quadVAO = 0;
    }
    if (m_quadVBO)
    {
        glDeleteBuffers(1, &m_quadVBO);
        m_quadVBO = 0;
    }
    if (m_shaderProgram)
    {
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }
    m_ready = false;
    LOG_INFO("[SplashScreen] Shutdown");
}

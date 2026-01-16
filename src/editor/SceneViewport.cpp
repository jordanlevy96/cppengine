/**
 * @file SceneViewport.cpp
 * @brief SceneViewport implementation
 */

#include "editor/SceneViewport.h"
#include "systems/RenderSystem.h"
#include "components/RenderComponent.h"
#include "components/Lighting.h"
#include "util/Logger.h"

#include <yaml-cpp/binary.h> // For base64 encoding
#include <fstream>

// stb_image_write for PNG encoding
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <cstring>
#include <sstream>

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
    if (!CreateFramebuffer())
    {
        LOG_ERROR("Failed to create framebuffer");
        return false;
    }

    m_initialized = true;
    LOG_INFO("SceneViewport initialized successfully");
    return true;
}

bool SceneViewport::CreateFramebuffer()
{
    // Create color texture
    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create depth renderbuffer
    glGenRenderbuffers(1, &m_depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_width, m_height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Create framebuffer
    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffer);

    // Verify framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG_ERROR("Framebuffer not complete: 0x{:x}", status);
        DeleteFramebuffer();
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    LOG_INFO("Framebuffer created successfully: {}x{}", m_width, m_height);
    return true;
}

void SceneViewport::DeleteFramebuffer()
{
    if (m_framebuffer != 0)
    {
        glDeleteFramebuffers(1, &m_framebuffer);
        m_framebuffer = 0;
    }
    if (m_colorTexture != 0)
    {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
    if (m_depthBuffer != 0)
    {
        glDeleteRenderbuffers(1, &m_depthBuffer);
        m_depthBuffer = 0;
    }
}

void SceneViewport::Render()
{
    if (!m_initialized)
    {
        LOG_ERROR("Cannot render - SceneViewport not initialized");
        return;
    }

    // Bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glViewport(0, 0, m_width, m_height);

    // Clear with dark gray background
    glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
            glm::vec3 originalColor = transform.Color;

            // Set bright highlight color (yellow/orange)
            transform.Color = glm::vec3(1.0f, 0.8f, 0.0f);

            // Enable wireframe mode for outline effect
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(3.0f);

            // Disable depth test so outline draws on top
            glDisable(GL_DEPTH_TEST);

            // Render entity again as wireframe
            RenderSystem::RenderEntity<RenderComponent>(m_selectedEntityId, m_camera);

            // Restore OpenGL state
            glEnable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glLineWidth(1.0f);

            // Restore original color
            transform.Color = originalColor;
        }
    }

    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        LOG_ERROR("OpenGL error after RenderSystem::Update: 0x{:x}", error);
    }

    // Unbind framebuffer (return to default framebuffer)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneViewport::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    LOG_INFO("Shutting down SceneViewport");
    DeleteFramebuffer();
    m_initialized = false;
}

void SceneViewport::Resize(int width, int height)
{
    if (width == m_width && height == m_height)
    {
        return;
    }

    LOG_INFO("Resizing SceneViewport: {}x{} -> {}x{}", m_width, m_height, width, height);

    m_width = width;
    m_height = height;

    // Update camera perspective
    m_camera->SetPerspective(m_camera->fov, static_cast<float>(width), static_cast<float>(height));

    // Recreate framebuffer
    DeleteFramebuffer();
    CreateFramebuffer();
}

std::string SceneViewport::GetTextureAsDataURI()
{
    if (!m_initialized)
    {
        LOG_ERROR("Cannot get texture - SceneViewport not initialized");
        return "";
    }

    // Read pixels from texture
    std::vector<uint8_t> pixels(m_width * m_height * 4);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        LOG_ERROR("OpenGL error reading texture: 0x{:x}", error);
        glBindTexture(GL_TEXTURE_2D, 0);
        return "";
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // Flip vertically (OpenGL textures are bottom-up, images are top-down)
    std::vector<uint8_t> flippedPixels(pixels.size());
    int rowSize = m_width * 4;
    for (int y = 0; y < m_height; y++)
    {
        std::memcpy(
            flippedPixels.data() + y * rowSize,
            pixels.data() + (m_height - 1 - y) * rowSize,
            rowSize);
    }

    // Encode as PNG + base64
    std::string dataURI = EncodePNGBase64(flippedPixels, m_width, m_height);

    static bool logged = false;
    if (!logged && !dataURI.empty())
    {
        LOG_INFO("Generated viewport data URI: {} bytes", dataURI.size());

        // DEBUG: Save viewport to file for inspection
        std::ofstream outFile("../viewport_debug.png", std::ios::binary);
        if (outFile.is_open())
        {
            // PNG encode without base64
            int pngSize;
            unsigned char *pngData = stbi_write_png_to_mem(
                flippedPixels.data(),
                m_width * 4,
                m_width,
                m_height,
                4,
                &pngSize);
            if (pngData)
            {
                outFile.write(reinterpret_cast<char *>(pngData), pngSize);
                STBIW_FREE(pngData);
                LOG_INFO("Saved viewport debug image to ../viewport_debug.png");
            }
            outFile.close();
        }

        logged = true;
    }

    return dataURI;
}

std::string SceneViewport::EncodePNGBase64(const std::vector<uint8_t> &pixels, int width, int height)
{
    // PNG encode using stb_image_write
    int pngSize;
    unsigned char *pngData = stbi_write_png_to_mem(
        pixels.data(),
        width * 4, // stride
        width,
        height,
        4, // RGBA
        &pngSize);

    if (!pngData)
    {
        LOG_ERROR("Failed to encode PNG");
        return "";
    }

    // Base64 encode using yaml-cpp's Binary class
    std::string base64 = YAML::EncodeBase64(pngData, pngSize);

    // Free stb_image_write allocated memory
    STBIW_FREE(pngData);

    // Return as data URI
    return "data:image/png;base64," + base64;
}

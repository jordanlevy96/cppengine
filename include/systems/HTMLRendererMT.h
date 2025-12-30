#pragma once

#include "util/Shader.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <litehtml.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <map>

// Multi-Threaded HTML Renderer
// Uses a background thread for HTML rendering and composites the result
class HTMLRendererMT {
public:
    static HTMLRendererMT &GetInstance() {
        static HTMLRendererMT instance;
        return instance;
    }

    HTMLRendererMT(HTMLRendererMT const &) = delete;
    void operator=(HTMLRendererMT const &) = delete;

    void Initialize(GLFWwindow *window, int width, int height);
    void Shutdown();
    void LoadHTML(const std::string& html);
    void UpdateHTML(const std::string& html);  // Same as LoadHTML but semantic
    void Render();
    void Resize(int width, int height);

private:
    HTMLRendererMT() = default;
    ~HTMLRendererMT() { Shutdown(); }

    // Thread management
    void RenderThreadLoop();
    void SetupGL();
    void UpdateTextureFromPixelBuffer();

    // Pixel buffer structure (replaces SharedFrameBuffer)
    struct FrameBuffer {
        uint32_t width;
        uint32_t height;
        uint32_t frameNumber;
        std::vector<uint8_t> pixels;  // RGBA
    };

    // Software renderer (runs on render thread)
    class SoftwareRenderer;

    GLFWwindow* m_window = nullptr;
    int m_width = 800;
    int m_height = 600;

    // Thread synchronization
    std::unique_ptr<std::thread> m_renderThread;
    std::atomic<bool> m_running{false};
    std::mutex m_mutex;
    std::condition_variable m_cv;

    // Shared state (protected by mutex)
    std::string m_pendingHTML;
    bool m_hasNewHTML = false;
    bool m_needsResize = false;
    int m_newWidth = 0;
    int m_newHeight = 0;

    // Frame buffers (double buffered)
    FrameBuffer m_frontBuffer;  // Read by main thread
    FrameBuffer m_backBuffer;   // Written by render thread
    std::mutex m_bufferMutex;
    uint32_t m_lastFrameNumber = 0;

    // OpenGL resources (main thread only)
    Shader* m_compositeShader = nullptr;
    unsigned int m_texture = 0;
    unsigned int m_quadVAO = 0;
    unsigned int m_quadVBO = 0;
    glm::mat4 m_projection;

    // Shutdown state
    bool m_isShutdown = false;
};

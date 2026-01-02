/**
 * @file HTMLRendererMT.h
 * @brief Multi-threaded HTML/CSS renderer using litehtml and FreeType
 *
 * Architecture: docs/architecture/MULTITHREADING.md
 */

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

/**
 * @brief Multi-threaded HTML/CSS renderer singleton
 *
 * Renders HTML/CSS on a background thread using litehtml + FreeType,
 * then composites result to screen via OpenGL texture. Prevents UI
 * rendering from blocking the main game loop.
 *
 * **Thread Safety:**
 * - Main thread: calls LoadHTML(), Render(), Resize()
 * - Render thread: internal rendering, writes to m_backBuffer
 * - Double buffering prevents race conditions
 *
 * **Performance:**
 * - HTML rendering: 5-15ms (async, doesn't block main thread)
 * - Main thread cost: ~2ms (texture upload + composite)
 *
 * @see docs/architecture/MULTITHREADING.md for implementation details
 */
class HTMLRendererMT {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to HTMLRendererMT singleton
     */
    static HTMLRendererMT &GetInstance() {
        static HTMLRendererMT instance;
        return instance;
    }

    HTMLRendererMT(HTMLRendererMT const &) = delete;
    void operator=(HTMLRendererMT const &) = delete;

    /**
     * @brief Initialize renderer and start background thread
     * @param window GLFW window for OpenGL context
     * @param width Initial viewport width
     * @param height Initial viewport height
     */
    void Initialize(GLFWwindow *window, int width, int height);

    /**
     * @brief Shutdown renderer and stop background thread
     * @note Blocks until render thread completes
     */
    void Shutdown();

    /**
     * @brief Queue HTML for rendering on background thread
     * @param html HTML/CSS string to render
     * @note Thread-safe, wakes render thread via condition variable
     */
    void LoadHTML(const std::string& html);

    /**
     * @brief Semantic alias for LoadHTML (update existing HTML)
     * @param html HTML/CSS string to render
     */
    void UpdateHTML(const std::string& html);

    /**
     * @brief Composite rendered UI texture to screen (call per frame)
     * @note Must be called from main thread with active OpenGL context
     */
    void Render();

    /**
     * @brief Update viewport size and signal render thread
     * @param width New viewport width
     * @param height New viewport height
     */
    void Resize(int width, int height);

    /**
     * @brief Handle mouse click event with hit-testing
     * @param x Mouse X coordinate (window space)
     * @param y Mouse Y coordinate (window space)
     * @param button Mouse button (0=left, 1=right, 2=middle)
     * @return true if click was handled by an interactive element
     * @note Thread-safe, uses m_frontInteractiveElements
     */
    bool HandleClickEvent(float x, float y, int button);

    /**
     * @brief Update hover state based on cursor position
     * @param x Mouse X coordinate (window space)
     * @param y Mouse Y coordinate (window space)
     * @note Generates synthetic mouseover/mouseout events
     */
    void UpdateHoverState(float x, float y);

    /**
     * @brief Get event handlers map from template parser
     * @return Map of element ID → {eventType → handlerExpression}
     * @note Set by ReactiveUI after template evaluation
     */
    const std::map<std::string, std::map<std::string, std::string>>& GetEventHandlers() const {
        return m_eventHandlers;
    }

    /**
     * @brief Set event handlers from template parser
     * @param handlers Map of element ID → {eventType → handlerExpression}
     * @note Called by ReactiveUI after template evaluation
     */
    void SetEventHandlers(const std::map<std::string, std::map<std::string, std::string>>& handlers) {
        m_eventHandlers = handlers;
    }

private:
    HTMLRendererMT() = default;
    ~HTMLRendererMT() { Shutdown(); }

    /**
     * @brief Main render thread loop (blocks until shutdown)
     * @note Runs on background thread, woken by m_cv
     */
    void RenderThreadLoop();

    /**
     * @brief Setup OpenGL state for UI compositing
     */
    void SetupGL();

    /**
     * @brief Upload pixel buffer to OpenGL texture (main thread)
     */
    void UpdateTextureFromPixelBuffer();

    /**
     * @brief Frame buffer for double buffering
     */
    struct FrameBuffer {
        uint32_t width;                 ///< Buffer width in pixels
        uint32_t height;                ///< Buffer height in pixels
        uint32_t frameNumber;           ///< Incremented on each render
        std::vector<uint8_t> pixels;    ///< RGBA pixel data
    };

    /**
     * @brief Interactive HTML element with event handlers
     */
    struct InteractiveElement {
        std::string id;                             ///< Unique element ID (data-event-id)
        int x, y, width, height;                    ///< Bounding box (screen coordinates)
        std::map<std::string, std::string> handlers; ///< Event type → handler expression
        int zIndex;                                 ///< CSS z-index for overlap resolution
    };

    /**
     * @brief Software renderer (runs on render thread)
     * @note Implements litehtml::document_container interface
     */
    class SoftwareRenderer;

    GLFWwindow* m_window = nullptr;     ///< GLFW window handle
    int m_width = 800;                  ///< Current viewport width
    int m_height = 600;                 ///< Current viewport height

    // Thread synchronization
    std::unique_ptr<std::thread> m_renderThread;    ///< Background render thread
    std::atomic<bool> m_running{false};             ///< Thread running flag
    std::mutex m_mutex;                             ///< Protects shared HTML/resize state
    std::condition_variable m_cv;                   ///< Wakes render thread on new work

    // Shared state (protected by m_mutex)
    std::string m_pendingHTML;          ///< HTML queued for rendering
    bool m_hasNewHTML = false;          ///< New HTML available flag
    bool m_needsResize = false;         ///< Resize pending flag
    int m_newWidth = 0;                 ///< New width for resize
    int m_newHeight = 0;                ///< New height for resize

    // Frame buffers (double buffered)
    FrameBuffer m_frontBuffer;          ///< Read by main thread (protected by m_bufferMutex)
    FrameBuffer m_backBuffer;           ///< Written by render thread (exclusive ownership)
    std::mutex m_bufferMutex;           ///< Protects buffer swap
    uint32_t m_lastFrameNumber = 0;

    // Interactive elements (double buffered for thread safety)
    std::vector<InteractiveElement> m_frontInteractiveElements;  ///< Read by main thread
    std::vector<InteractiveElement> m_backInteractiveElements;   ///< Written by render thread
    std::mutex m_interactiveElementsMutex;                       ///< Protects element swap
    std::map<std::string, std::map<std::string, std::string>> m_eventHandlers; ///< From TemplateParser
    std::string m_lastHoveredElement;                            ///< Track hover state for mouseout events

    // OpenGL resources (main thread only)
    Shader* m_compositeShader = nullptr;
    unsigned int m_texture = 0;
    unsigned int m_quadVAO = 0;
    unsigned int m_quadVBO = 0;
    glm::mat4 m_projection;

    // Shutdown state
    bool m_isShutdown = false;
};

#pragma once

#include "systems/HTMLRenderIPC.h"
#include "systems/SharedMemory.h"
#include "systems/UnixSocket.h"
#include "systems/IPCClient.h"
#include "util/Shader.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <memory>
#include <sys/types.h>

// Multi-Process HTML Renderer
// Spawns a separate process for HTML rendering and composites the result
class HTMLRendererMP {
public:
    static HTMLRendererMP &GetInstance() {
        static HTMLRendererMP instance;
        return instance;
    }

    HTMLRendererMP(HTMLRendererMP const &) = delete;
    void operator=(HTMLRendererMP const &) = delete;

    void Initialize(GLFWwindow *window, int width, int height);
    void Shutdown();
    void LoadHTML(const std::string& html);
    void UpdateHTML(const std::string& html);  // Update existing HTML (same as LoadHTML but semantic)
    void Render();
    void Resize(int width, int height);

private:
    HTMLRendererMP() = default;

    void SpawnRenderProcess();
    void SetupGL();
    void UpdateTextureFromSharedMemory();

    GLFWwindow* m_window = nullptr;
    int m_width = 800;
    int m_height = 600;

    // Process management
    pid_t m_renderProcessPID = -1;
    std::string m_shmName;
    std::string m_socketPath;

    // IPC
    std::unique_ptr<SharedMemory> m_sharedMemory;
    std::unique_ptr<UnixSocket> m_serverSocket;  // Server socket for accepting connection
    std::unique_ptr<IPCClient> m_ipcClient;      // Client for sending messages

    // OpenGL resources
    Shader* m_compositeShader = nullptr;
    unsigned int m_texture = 0;
    unsigned int m_quadVAO = 0;
    unsigned int m_quadVBO = 0;
    glm::mat4 m_projection;

    uint32_t m_lastFrameNumber = 0;

    // Track base HTML template for dynamic updates
    std::string m_htmlTemplate;
};

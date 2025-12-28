#include <glad/glad.h>
#include "systems/HTMLRendererMP.h"

#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

void HTMLRendererMP::Initialize(GLFWwindow *window, int width, int height) {
    m_window = window;
    m_width = width;
    m_height = height;

    // Generate unique names for shared resources
    m_shmName = "/html_render_" + std::to_string(getpid());
    m_socketPath = "/tmp/html_render_" + std::to_string(getpid()) + ".sock";

    std::cout << "[HTMLRendererMP] Initializing multi-process HTML renderer" << std::endl;
    std::cout << "[HTMLRendererMP] SHM: " << m_shmName << std::endl;
    std::cout << "[HTMLRendererMP] Socket: " << m_socketPath << std::endl;

    try {
        // Create shared memory
        m_sharedMemory = std::make_unique<SharedMemory>(m_shmName, width, height, true);

        // Create IPC socket (server side)
        m_serverSocket = std::make_unique<UnixSocket>();
        m_serverSocket->Bind(m_socketPath);

        // Spawn render process
        SpawnRenderProcess();

        // Accept connection from render process
        std::cout << "[HTMLRendererMP] Waiting for render process connection..." << std::endl;
        std::cout.flush();

        int clientFD = m_serverSocket->Accept();
        std::cout << "[HTMLRendererMP] Render process connected (FD: " << clientFD << ")" << std::endl;
        std::cout.flush();

        // Wrap the client FD in our IPCClient
        m_ipcClient = std::make_unique<IPCClient>(clientFD);

        // Wait for READY message
        IPCMessage readyMsg;
        if (m_ipcClient->TryReceiveMessage(readyMsg)) {
            std::cout << "[HTMLRendererMP] Received READY from render process" << std::endl;
        } else {
            std::cout << "[HTMLRendererMP] No READY message (proceeding anyway)" << std::endl;
        }
        std::cout.flush();

        // Set up OpenGL resources
        SetupGL();

        m_projection = glm::ortho(0.0f, (float)m_width, (float)m_height, 0.0f, -1.0f, 1.0f);

        std::cout << "[HTMLRendererMP] Initialization complete" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "[HTMLRendererMP] Initialization failed: " << e.what() << std::endl;
        Shutdown();
        throw;
    }
}

void HTMLRendererMP::SpawnRenderProcess() {
    pid_t pid = fork();

    if (pid == -1) {
        throw std::runtime_error("Failed to fork render process");
    }
    else if (pid == 0) {
        // Child process
        std::string sizeStr = std::to_string(m_width) + "x" + std::to_string(m_height);

        // Execute render process
        execl("./html_render_process",
              "html_render_process",
              m_shmName.c_str(),
              m_socketPath.c_str(),
              sizeStr.c_str(),
              nullptr);

        // If execl returns, it failed
        std::cerr << "[HTMLRendererMP] Failed to exec render process: " << strerror(errno) << std::endl;
        exit(1);
    }
    else {
        // Parent process
        m_renderProcessPID = pid;
        std::cout << "[HTMLRendererMP] Spawned render process with PID " << pid << std::endl;
    }
}

void HTMLRendererMP::SetupGL() {
    // Create composite shader
    m_compositeShader = new Shader("../res/shaders/Composite.shader");

    // Create texture for uploading shared memory pixels
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create quad for rendering texture (screen space coordinates)
    float quadVertices[] = {
        // positions (screen space)        // texCoords
        0.0f,           0.0f,               0.0f, 1.0f,  // top-left
        0.0f,           (float)m_height,    0.0f, 0.0f,  // bottom-left
        (float)m_width, (float)m_height,    1.0f, 0.0f,  // bottom-right

        0.0f,           0.0f,               0.0f, 1.0f,  // top-left
        (float)m_width, (float)m_height,    1.0f, 0.0f,  // bottom-right
        (float)m_width, 0.0f,               1.0f, 1.0f   // top-right
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void HTMLRendererMP::LoadHTML(const std::string& html) {
    if (!m_ipcClient) {
        std::cerr << "[HTMLRendererMP] Cannot load HTML: IPC not initialized" << std::endl;
        return;
    }

    // Store the template for potential updates
    m_htmlTemplate = html;

    std::cout << "[HTMLRendererMP] Sending LOAD_HTML message (" << html.size() << " bytes)" << std::endl;
    std::cout.flush();

    IPCMessage msg = CreateLoadHTMLMessage(html);

    if (m_ipcClient->SendMessage(msg)) {
        std::cout << "[HTMLRendererMP] LOAD_HTML message sent successfully" << std::endl;
    } else {
        std::cerr << "[HTMLRendererMP] Failed to send LOAD_HTML message" << std::endl;
    }
    std::cout.flush();
}

void HTMLRendererMP::UpdateHTML(const std::string& html) {
    // Same as LoadHTML - the render process will re-render
    LoadHTML(html);
}

void HTMLRendererMP::Render() {
    if (!m_sharedMemory || !m_compositeShader) return;

    SharedFrameBuffer* buffer = m_sharedMemory->GetBuffer();

    // Check if new frame is ready
    if (buffer->ready && buffer->frameNumber != m_lastFrameNumber) {
        UpdateTextureFromSharedMemory();
        m_lastFrameNumber = buffer->frameNumber;
    }

    // Set up OpenGL state
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use composite shader
    m_compositeShader->Use();
    m_compositeShader->SetMat4("projection", m_projection);
    m_compositeShader->SetInt("screenTexture", 0);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    // Draw quad with texture
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Cleanup
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Restore OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void HTMLRendererMP::UpdateTextureFromSharedMemory() {
    SharedFrameBuffer* buffer = m_sharedMemory->GetBuffer();

    // Upload pixels to texture
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height,
                    GL_RGBA, GL_UNSIGNED_BYTE, buffer->pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void HTMLRendererMP::Resize(int width, int height) {
    if (!m_ipcClient) return;

    m_width = width;
    m_height = height;

    // Update projection matrix
    m_projection = glm::ortho(0.0f, (float)m_width, (float)m_height, 0.0f, -1.0f, 1.0f);

    // Send resize message to render process
    IPCMessage msg = CreateResizeMessage(width, height);
    m_ipcClient->SendMessage(msg);

    // Recreate texture
    if (m_texture) {
        glDeleteTextures(1, &m_texture);
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Update quad vertices for new size
    float quadVertices[] = {
        0.0f,           0.0f,               0.0f, 1.0f,
        0.0f,           (float)m_height,    0.0f, 0.0f,
        (float)m_width, (float)m_height,    1.0f, 0.0f,

        0.0f,           0.0f,               0.0f, 1.0f,
        (float)m_width, (float)m_height,    1.0f, 0.0f,
        (float)m_width, 0.0f,               1.0f, 1.0f
    };

    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quadVertices), quadVertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void HTMLRendererMP::Shutdown() {
    std::cout << "[HTMLRendererMP] Shutting down" << std::endl;
    std::cout.flush();

    // Send shutdown message to render process
    if (m_ipcClient) {
        IPCMessage msg = CreateShutdownMessage();
        m_ipcClient->SendMessage(msg);
        m_ipcClient.reset();  // Closes the socket FD
    }

    // Wait for render process to exit
    if (m_renderProcessPID != -1) {
        int status;
        waitpid(m_renderProcessPID, &status, 0);
        m_renderProcessPID = -1;
    }

    // Clean up OpenGL resources
    if (m_texture) glDeleteTextures(1, &m_texture);
    if (m_quadVAO) glDeleteVertexArrays(1, &m_quadVAO);
    if (m_quadVBO) glDeleteBuffers(1, &m_quadVBO);
    if (m_compositeShader) delete m_compositeShader;

    // Clean up IPC
    if (m_serverSocket) {
        m_serverSocket->Close();
        m_serverSocket.reset();
    }

    // Clean up shared memory
    if (m_sharedMemory) {
        m_sharedMemory->Unlink();
        m_sharedMemory.reset();
    }

    std::cout << "[HTMLRendererMP] Shutdown complete" << std::endl;
}

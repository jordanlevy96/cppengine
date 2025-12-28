#pragma once

#include "systems/HTMLRenderIPC.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <string>

class UnixSocket {
public:
    UnixSocket() : m_fd(-1) {}

    ~UnixSocket() {
        Close();
    }

    UnixSocket(const UnixSocket&) = delete;
    UnixSocket& operator=(const UnixSocket&) = delete;

    // Server side: create and bind socket
    void Bind(const std::string& path) {
        m_path = path;

        // Remove existing socket file
        unlink(path.c_str());

        m_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_fd == -1) {
            throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
        }

        struct sockaddr_un addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

        if (bind(m_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            close(m_fd);
            m_fd = -1;
            throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
        }

        if (listen(m_fd, 1) == -1) {
            close(m_fd);
            m_fd = -1;
            throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
        }
    }

    // Server side: accept connection
    int Accept() {
        if (m_fd == -1) {
            throw std::runtime_error("Socket not bound");
        }

        int client_fd = accept(m_fd, nullptr, nullptr);
        if (client_fd == -1) {
            throw std::runtime_error("Failed to accept connection: " + std::string(strerror(errno)));
        }

        return client_fd;
    }

    // Client side: connect to socket
    void Connect(const std::string& path) {
        m_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_fd == -1) {
            throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
        }

        struct sockaddr_un addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

        if (connect(m_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            close(m_fd);
            m_fd = -1;
            throw std::runtime_error("Failed to connect to socket: " + std::string(strerror(errno)));
        }
    }

    // Send message
    bool SendMessage(const IPCMessage& msg) {
        if (m_fd == -1) {
            return false;
        }

        size_t totalSize = sizeof(MessageHeader) + msg.header.payloadSize;
        ssize_t sent = send(m_fd, &msg, totalSize, 0);

        return sent == (ssize_t)totalSize;
    }

    // Receive message (blocking)
    bool ReceiveMessage(IPCMessage& msg) {
        if (m_fd == -1) {
            return false;
        }

        // First receive header
        ssize_t received = recv(m_fd, &msg.header, sizeof(MessageHeader), MSG_WAITALL);
        if (received != sizeof(MessageHeader)) {
            return false;
        }

        // Then receive payload if any
        if (msg.header.payloadSize > 0) {
            size_t payloadSize = std::min(msg.header.payloadSize,
                                         (uint32_t)(MAX_MESSAGE_SIZE - sizeof(MessageHeader)));
            received = recv(m_fd, &msg.payload, payloadSize, MSG_WAITALL);
            if (received != (ssize_t)payloadSize) {
                return false;
            }
        }

        return true;
    }

    // Non-blocking receive
    bool TryReceiveMessage(IPCMessage& msg) {
        if (m_fd == -1) {
            return false;
        }

        // Set non-blocking
        int flags = fcntl(m_fd, F_GETFL, 0);
        fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);

        ssize_t received = recv(m_fd, &msg.header, sizeof(MessageHeader), 0);

        // Restore blocking
        fcntl(m_fd, F_SETFL, flags);

        if (received != sizeof(MessageHeader)) {
            return false;
        }

        // Receive payload if any
        if (msg.header.payloadSize > 0) {
            size_t payloadSize = std::min(msg.header.payloadSize,
                                         (uint32_t)(MAX_MESSAGE_SIZE - sizeof(MessageHeader)));
            received = recv(m_fd, &msg.payload, payloadSize, MSG_WAITALL);
            if (received != (ssize_t)payloadSize) {
                return false;
            }
        }

        return true;
    }

    void Close() {
        if (m_fd != -1) {
            close(m_fd);
            m_fd = -1;
        }
        if (!m_path.empty()) {
            unlink(m_path.c_str());
            m_path.clear();
        }
    }

    int GetFD() const { return m_fd; }

private:
    int m_fd;
    std::string m_path;
};

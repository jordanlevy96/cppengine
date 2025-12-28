#pragma once

#include "systems/HTMLRenderIPC.h"
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

// Encapsulates IPC communication with the render process
class IPCClient {
public:
    IPCClient(int socketFD) : m_fd(socketFD) {}

    ~IPCClient() {
        if (m_fd != -1) {
            close(m_fd);
        }
    }

    bool SendMessage(const IPCMessage& msg) {
        if (m_fd == -1) {
            std::cerr << "[IPCClient] Socket not connected" << std::endl;
            return false;
        }

        size_t totalSize = sizeof(MessageHeader) + msg.header.payloadSize;

        std::cout << "[IPCClient] Sending message type " << (int)msg.header.type
                  << " size " << totalSize << std::endl;
        std::cout.flush();

        ssize_t sent = send(m_fd, &msg, totalSize, 0);

        std::cout << "[IPCClient] Send returned: " << sent << std::endl;
        std::cout.flush();

        if (sent != (ssize_t)totalSize) {
            std::cerr << "[IPCClient] Send failed: " << strerror(errno) << std::endl;
            return false;
        }

        return true;
    }

    // Non-blocking receive for responses
    bool TryReceiveMessage(IPCMessage& msg) {
        if (m_fd == -1) return false;

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

    int GetFD() const { return m_fd; }

private:
    int m_fd;
};

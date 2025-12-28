#pragma once

#include <cstdint>
#include <cstring>
#include <string>

// Maximum size for messages
constexpr size_t MAX_MESSAGE_SIZE = 4096;
constexpr size_t MAX_HTML_SIZE = 1024 * 1024; // 1MB for HTML content

// Shared memory frame buffer structure
struct SharedFrameBuffer {
    // Metadata
    uint32_t width;
    uint32_t height;
    uint32_t frameNumber;
    volatile uint8_t ready;  // 0 = being written, 1 = ready to read
    uint8_t padding[3];

    // Pixel data follows (RGBA, size = width * height * 4)
    // Allocated dynamically based on size
    uint8_t pixels[];
};

// Calculate total shared memory size needed
inline size_t GetSharedMemorySize(uint32_t width, uint32_t height) {
    return sizeof(SharedFrameBuffer) + (width * height * 4);
}

// IPC Message types
enum class MessageType : uint32_t {
    LOAD_HTML = 1,
    RESIZE = 2,
    SHUTDOWN = 3,
    MARK_DIRTY = 4,
    READY = 100,
    ERROR = 101
};

// Base message header
struct MessageHeader {
    MessageType type;
    uint32_t payloadSize;
};

// LOAD_HTML message payload
struct LoadHTMLPayload {
    uint32_t htmlSize;
    char html[MAX_HTML_SIZE];
};

// RESIZE message payload
struct ResizePayload {
    uint32_t width;
    uint32_t height;
};

// ERROR message payload
struct ErrorPayload {
    char message[256];
};

// Complete message structure
struct IPCMessage {
    MessageHeader header;
    union {
        LoadHTMLPayload loadHTML;
        ResizePayload resize;
        ErrorPayload error;
        uint8_t rawPayload[MAX_MESSAGE_SIZE - sizeof(MessageHeader)];
    } payload;
};

// Helper functions for creating messages
inline IPCMessage CreateLoadHTMLMessage(const std::string& html) {
    IPCMessage msg;
    msg.header.type = MessageType::LOAD_HTML;

    size_t htmlSize = std::min(html.size(), (size_t)MAX_HTML_SIZE - 1);
    msg.payload.loadHTML.htmlSize = htmlSize;
    std::strncpy(msg.payload.loadHTML.html, html.c_str(), htmlSize);
    msg.payload.loadHTML.html[htmlSize] = '\0';

    // Only send the actual HTML size + the htmlSize field, not the full MAX_HTML_SIZE buffer
    msg.header.payloadSize = sizeof(uint32_t) + htmlSize + 1;
    return msg;
}

inline IPCMessage CreateResizeMessage(uint32_t width, uint32_t height) {
    IPCMessage msg;
    msg.header.type = MessageType::RESIZE;
    msg.payload.resize.width = width;
    msg.payload.resize.height = height;
    msg.header.payloadSize = sizeof(ResizePayload);
    return msg;
}

inline IPCMessage CreateShutdownMessage() {
    IPCMessage msg;
    msg.header.type = MessageType::SHUTDOWN;
    msg.header.payloadSize = 0;
    return msg;
}

inline IPCMessage CreateMarkDirtyMessage() {
    IPCMessage msg;
    msg.header.type = MessageType::MARK_DIRTY;
    msg.header.payloadSize = 0;
    return msg;
}

inline IPCMessage CreateReadyMessage() {
    IPCMessage msg;
    msg.header.type = MessageType::READY;
    msg.header.payloadSize = 0;
    return msg;
}

inline IPCMessage CreateErrorMessage(const std::string& errorMsg) {
    IPCMessage msg;
    msg.header.type = MessageType::ERROR;
    std::strncpy(msg.payload.error.message, errorMsg.c_str(), 255);
    msg.payload.error.message[255] = '\0';
    msg.header.payloadSize = sizeof(ErrorPayload);
    return msg;
}

#pragma once

#include "systems/HTMLRenderIPC.h"
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

class SharedMemory {
public:
    SharedMemory(const std::string& name, uint32_t width, uint32_t height, bool create)
        : m_name(name), m_width(width), m_height(height), m_fd(-1), m_buffer(nullptr)
    {
        m_size = GetSharedMemorySize(width, height);

        if (create) {
            CreateSharedMemory();
        } else {
            AttachSharedMemory();
        }
    }

    ~SharedMemory() {
        Cleanup();
    }

    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;

    SharedFrameBuffer* GetBuffer() { return m_buffer; }
    const std::string& GetName() const { return m_name; }
    size_t GetSize() const { return m_size; }

    void Unlink() {
        shm_unlink(m_name.c_str());
    }

private:
    void CreateSharedMemory() {
        // Create shared memory object
        m_fd = shm_open(m_name.c_str(), O_CREAT | O_RDWR, 0666);
        if (m_fd == -1) {
            throw std::runtime_error("Failed to create shared memory: " + std::string(strerror(errno)));
        }

        // Set size
        if (ftruncate(m_fd, m_size) == -1) {
            close(m_fd);
            shm_unlink(m_name.c_str());
            throw std::runtime_error("Failed to set shared memory size: " + std::string(strerror(errno)));
        }

        // Map to address space
        void* addr = mmap(nullptr, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
        if (addr == MAP_FAILED) {
            close(m_fd);
            shm_unlink(m_name.c_str());
            throw std::runtime_error("Failed to map shared memory: " + std::string(strerror(errno)));
        }

        m_buffer = static_cast<SharedFrameBuffer*>(addr);

        // Initialize buffer
        m_buffer->width = m_width;
        m_buffer->height = m_height;
        m_buffer->frameNumber = 0;
        m_buffer->ready = 0;
        std::memset(m_buffer->pixels, 0, m_width * m_height * 4);
    }

    void AttachSharedMemory() {
        // Open existing shared memory
        m_fd = shm_open(m_name.c_str(), O_RDWR, 0666);
        if (m_fd == -1) {
            throw std::runtime_error("Failed to open shared memory: " + std::string(strerror(errno)));
        }

        // Map to address space
        void* addr = mmap(nullptr, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
        if (addr == MAP_FAILED) {
            close(m_fd);
            throw std::runtime_error("Failed to map shared memory: " + std::string(strerror(errno)));
        }

        m_buffer = static_cast<SharedFrameBuffer*>(addr);
    }

    void Cleanup() {
        if (m_buffer != nullptr) {
            munmap(m_buffer, m_size);
            m_buffer = nullptr;
        }
        if (m_fd != -1) {
            close(m_fd);
            m_fd = -1;
        }
    }

    std::string m_name;
    uint32_t m_width;
    uint32_t m_height;
    size_t m_size;
    int m_fd;
    SharedFrameBuffer* m_buffer;
};

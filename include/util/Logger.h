#pragma once

#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include <string>

namespace cppengine {

// Singleton logger manager
class Logger {
public:
    static Logger& GetInstance();
    void Initialize(const std::string& log_file = "logs/cppengine.log");
    quill::Logger* GetLogger();
    void Shutdown();

private:
    Logger() = default;
    ~Logger() { Shutdown(); }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    quill::Logger* m_logger = nullptr;
    bool m_initialized = false;
};

} // namespace cppengine

// Convenience macros - wraps Quill macros with singleton logger
#define LOG_TRACE_L3(...) QUILL_LOG_TRACE_L3(cppengine::Logger::GetInstance().GetLogger(), __VA_ARGS__)
#define LOG_TRACE_L2(...) QUILL_LOG_TRACE_L2(cppengine::Logger::GetInstance().GetLogger(), __VA_ARGS__)
#define LOG_TRACE_L1(...) QUILL_LOG_TRACE_L1(cppengine::Logger::GetInstance().GetLogger(), __VA_ARGS__)
#define LOG_DEBUG(...) QUILL_LOG_DEBUG(cppengine::Logger::GetInstance().GetLogger(), __VA_ARGS__)
#define LOG_INFO(...) QUILL_LOG_INFO(cppengine::Logger::GetInstance().GetLogger(), __VA_ARGS__)
#define LOG_WARNING(...) QUILL_LOG_WARNING(cppengine::Logger::GetInstance().GetLogger(), __VA_ARGS__)
#define LOG_ERROR(...) QUILL_LOG_ERROR(cppengine::Logger::GetInstance().GetLogger(), __VA_ARGS__)
#define LOG_CRITICAL(...) QUILL_LOG_CRITICAL(cppengine::Logger::GetInstance().GetLogger(), __VA_ARGS__)

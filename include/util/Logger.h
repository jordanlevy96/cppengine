/**
 * @file Logger.h
 * @brief High-performance async logging system using Quill
 */

#pragma once

#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include <string>

namespace imhotep
{

    /**
     * @brief Singleton logger manager wrapping Quill logging library
     *
     * Provides thread-safe async logging with ~12-16μs latency.
     * Automatically initialized on first use. Outputs to both console and file.
     *
     * @note Use LOG_* macros instead of calling GetLogger() directly
     */
    class Logger
    {
    public:
        /**
         * @brief Get singleton instance
         * @return Reference to Logger singleton
         */
        static Logger &GetInstance();

        /**
         * @brief Initialize logging system
         * @param log_file Path to log file (default: logs/imhotep.log)
         * @note Safe to call multiple times, only initializes once
         * @note Must be called before any LOG_* macros
         * @return True if successful
         */
        bool Initialize(const std::string &log_file = "logs/imhotep.log");

        /**
         * @brief Get underlying Quill logger instance
         * @return Pointer to Quill logger (nullptr if not initialized)
         */
        quill::Logger *GetLogger();

        /**
         * @brief Shutdown logging system and flush all logs
         */
        void Shutdown();

    private:
        Logger() = default;
        ~Logger() = default;

        Logger(const Logger &) = delete;
        Logger &operator=(const Logger &) = delete;

        quill::Logger *m_logger = nullptr; ///< Underlying Quill logger instance
        bool m_initialized = false;        ///< Initialization flag
    };

} // namespace imhotep

/**
 * @defgroup LogMacros Logging Macros
 * @brief Convenience macros for logging with automatic formatting
 *
 * All macros support printf-style formatting via fmt library:
 * @code
 * LOG_INFO("Loaded {} resources in {}ms", count, duration);
 * @endcode
 *
 * @{
 */

/// @brief Trace logging level 3 (most verbose)
#define LOG_TRACE_L3(...) QUILL_LOG_TRACE_L3(imhotep::Logger::GetInstance().GetLogger(), __VA_ARGS__)

/// @brief Trace logging level 2
#define LOG_TRACE_L2(...) QUILL_LOG_TRACE_L2(imhotep::Logger::GetInstance().GetLogger(), __VA_ARGS__)

/// @brief Trace logging level 1 (least verbose trace)
#define LOG_TRACE_L1(...) QUILL_LOG_TRACE_L1(imhotep::Logger::GetInstance().GetLogger(), __VA_ARGS__)

/// @brief Debug messages for development
#define LOG_DEBUG(...) QUILL_LOG_DEBUG(imhotep::Logger::GetInstance().GetLogger(), __VA_ARGS__)

/// @brief Informational messages
#define LOG_INFO(...) QUILL_LOG_INFO(imhotep::Logger::GetInstance().GetLogger(), __VA_ARGS__)

/// @brief Warning messages for recoverable issues
#define LOG_WARNING(...) QUILL_LOG_WARNING(imhotep::Logger::GetInstance().GetLogger(), __VA_ARGS__)

/// @brief Error messages for failures
#define LOG_ERROR(...) QUILL_LOG_ERROR(imhotep::Logger::GetInstance().GetLogger(), __VA_ARGS__)

/// @brief Critical errors requiring immediate attention
#define LOG_CRITICAL(...) QUILL_LOG_CRITICAL(imhotep::Logger::GetInstance().GetLogger(), __VA_ARGS__)

/** @} */

#include "util/Logger.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/sinks/ConsoleSink.h"
#include "quill/sinks/FileSink.h"
#include <algorithm>
#include <cctype>

namespace imhotep
{
    namespace
    {
#ifndef IMHOTEP_LOG_LEVEL_STR
#define IMHOTEP_LOG_LEVEL_STR "Info"
#endif

        quill::LogLevel ParseLogLevel(const char *levelStr)
        {
            if (levelStr == nullptr)
            {
                return quill::LogLevel::Info;
            }

            std::string level(levelStr);
            std::transform(level.begin(), level.end(), level.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            if (level == "tracel3" || level == "trace3" || level == "trace_l3")
            {
                return quill::LogLevel::TraceL3;
            }
            if (level == "tracel2" || level == "trace2" || level == "trace_l2")
            {
                return quill::LogLevel::TraceL2;
            }
            if (level == "tracel1" || level == "trace1" || level == "trace_l1")
            {
                return quill::LogLevel::TraceL1;
            }
            if (level == "debug")
            {
                return quill::LogLevel::Debug;
            }
            if (level == "info")
            {
                return quill::LogLevel::Info;
            }
            if (level == "warning" || level == "warn")
            {
                return quill::LogLevel::Warning;
            }
            if (level == "error")
            {
                return quill::LogLevel::Error;
            }
            if (level == "critical")
            {
                return quill::LogLevel::Critical;
            }
            if (level == "off" || level == "none")
            {
                return quill::LogLevel::None;
            }

            return quill::LogLevel::Info;
        }
    } // namespace

    Logger &Logger::GetInstance()
    {
        static Logger instance;
        return instance;
    }

    bool Logger::Initialize(const std::string &log_file)
    {
        if (m_initialized)
        {
            // Already initialized, just return true
            // This allows multiple initialization calls without error
            return true;
        }

        // Start the backend thread
        quill::BackendOptions backend_options;
        quill::Backend::start(backend_options);

        // Create console sink
        auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console");

        // Create file sink
        auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
            log_file,
            []()
            {
                quill::FileSinkConfig config;
                config.set_open_mode('w'); // Overwrite on each run
                config.set_filename_append_option(quill::FilenameAppendOption::None);
                return config;
            }(),
            quill::FileEventNotifier{});

        // Create logger with both console and file output
        m_logger = quill::Frontend::create_or_get_logger(
            "root",
            {std::move(console_sink), std::move(file_sink)});

        m_logger->set_log_level(ParseLogLevel(IMHOTEP_LOG_LEVEL_STR));

        m_initialized = true;
        return true;
    }

    quill::Logger *Logger::GetLogger()
    {
        return m_logger;
    }

    void Logger::Shutdown()
    {
        if (!m_initialized)
            return;
        quill::Backend::stop();
        m_initialized = false;
    }

} // namespace imhotep

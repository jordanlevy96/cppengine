#include "util/Logger.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/sinks/ConsoleSink.h"
#include "quill/sinks/FileSink.h"

namespace imhotep
{

    Logger &Logger::GetInstance()
    {
        static Logger instance;
        return instance;
    }

    bool Logger::Initialize(const std::string &log_file)
    {
        if (m_initialized)
        {
            LOG_CRITICAL("Logger already initialized");
            return false;
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

        // TODO: Set log level from config
        m_logger->set_log_level(quill::LogLevel::TraceL3);

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

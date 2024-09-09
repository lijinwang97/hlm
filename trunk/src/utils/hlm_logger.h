#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>

class Logger {
   public:
    enum class LogLevel {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical,
        Off
    };

    enum class OutputTarget {
        Console,
        File,
        Both
    };

    static void init(LogLevel level = LogLevel::Info,
                     OutputTarget target = OutputTarget::Both,
                     const std::string& base_name = "logs",
                     bool useAsync = false,
                     std::size_t maxFileSize = 104857600,  // 100 MB
                     std::size_t maxFiles = 3);

    static void setLogLevel(LogLevel level);

    template <typename... Args>
    static void TRACE(const char* fmt, const Args&... args) {
        logger->trace(fmt, args...);
    }

    template <typename... Args>
    static void DEBUG(const char* fmt, const Args&... args) {
        logger->debug(fmt, args...);
    }

    template <typename... Args>
    static void INFO(const char* fmt, const Args&... args) {
        logger->info(fmt, args...);
    }

    template <typename... Args>
    static void WARN(const char* fmt, const Args&... args) {
        logger->warn(fmt, args...);
    }

    template <typename... Args>
    static void ERROR(const char* fmt, const Args&... args) {
        logger->error(fmt, args...);
    }

    template <typename... Args>
    static void CRITICAL(const char* fmt, const Args&... args) {
        logger->critical(fmt, args...);
    }

   private:
    static std::shared_ptr<spdlog::logger> logger;

    static std::string generateLogFileName(const std::string& base_name);
};

#define hlm_trace(...) Logger::TRACE(__VA_ARGS__)
#define hlm_debug(...) Logger::DEBUG(__VA_ARGS__)
#define hlm_info(...) Logger::INFO(__VA_ARGS__)
#define hlm_warn(...) Logger::WARN(__VA_ARGS__)
#define hlm_error(...) Logger::ERROR(__VA_ARGS__)
#define hlm_critical(...) Logger::CRITICAL(__VA_ARGS__)

#endif  // LOGGER_H

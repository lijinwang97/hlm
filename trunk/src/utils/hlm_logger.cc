#include "hlm_logger.h"

#include <spdlog/async.h>
#include <spdlog/async_logger.h>

#include <chrono>
#include <iomanip>
#include <sstream>

shared_ptr<spdlog::logger> Logger::logger = nullptr;

void Logger::init(LogLevel level,
                  OutputTarget target,
                  const string& dir,
                  const string& base_name,
                  bool useAsync,
                  size_t maxFileSize,
                  size_t maxFiles) {
    vector<spdlog::sink_ptr> sinks;
    if (target == OutputTarget::Console || target == OutputTarget::Both) {
        sinks.push_back(make_shared<spdlog::sinks::stdout_color_sink_mt>());
    }

    if (target == OutputTarget::File || target == OutputTarget::Both) {
        string logFileName = dir + "/" + generateLogFileName(base_name);
        sinks.push_back(make_shared<spdlog::sinks::rotating_file_sink_mt>(logFileName, maxFileSize, maxFiles));
    }

    if (useAsync) {
        spdlog::init_thread_pool(8192, 1);
        logger = make_shared<spdlog::async_logger>("logger", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    } else {
        logger = make_shared<spdlog::logger>("logger", sinks.begin(), sinks.end());
    }

    spdlog::set_default_logger(logger);
    setLogLevel(level);
    spdlog::set_pattern("%^[%Y-%m-%d %H:%M:%S] [%t] [%l] %v%$");
}

void Logger::setLogLevel(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:
            logger->set_level(spdlog::level::trace);
            break;
        case LogLevel::Debug:
            logger->set_level(spdlog::level::debug);
            break;
        case LogLevel::Info:
            logger->set_level(spdlog::level::info);
            break;
        case LogLevel::Warn:
            logger->set_level(spdlog::level::warn);
            break;
        case LogLevel::Error:
            logger->set_level(spdlog::level::err);
            break;
        case LogLevel::Critical:
            logger->set_level(spdlog::level::critical);
            break;
        case LogLevel::Off:
            logger->set_level(spdlog::level::off);
            break;
    }
}

string Logger::generateLogFileName(const string& base_name) {
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    tm local_time = *localtime(&now_time);

    ostringstream oss;
    oss << base_name << "_"
        << put_time(&local_time, "%Y%m%d_%H%M%S")
        << ".log";
    return oss.str();
}

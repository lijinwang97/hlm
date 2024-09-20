#pragma once

#include <string>

#include "hlm_logger.h"

struct FileConfig {
    std::string input_file;
    std::string output_file;
};

struct HttpConfig {
    int port;
};

struct TaskConfig {
    int max_tasks;
};

struct LoggerConfig {
    std::string level;
    std::string target;
    std::string dir;
    std::string base_name;
    bool use_async;
    std::size_t max_file_size;
    std::size_t max_files;
};

struct VideoConfig {
    std::string codec;
    int bitrate;
    int width;
    int height;
    int framerate;
    int gop_size;
};

struct AudioConfig {
    std::string codec;
    int bitrate;
    int sample_rate;
    int channels;
};

class Config {
   public:
    static Config& getInstance();
    static void load(const std::string& config_file);
    void printAllConfigs() const;

    // Http Config Accessors
    const int getHttpPort() const { return http_config.port; }

    // Task Config Accessors
    const int getMaxTasks() const { return task_config.max_tasks; }

    // Logger Config Accessors
    Logger::LogLevel getLogLevel() const { return parseLogLevel(logger_config.level); }
    Logger::OutputTarget getLogTarget() const { return parseOutputTarget(logger_config.target); }
    const std::string& getLogDir() const { return logger_config.dir; }
    const std::string& getLogBaseName() const { return logger_config.base_name; }
    bool useAsyncLogging() const { return logger_config.use_async; }
    std::size_t getLogMaxFileSize() const { return logger_config.max_file_size * 1024 * 1024; }
    std::size_t getLogMaxFiles() const { return logger_config.max_files; }

   private:
    using LogLevel = Logger::LogLevel;
    using OutputTarget = Logger::OutputTarget;

    Config() = default;  // 构造函数私有化
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    static HttpConfig http_config;
    static TaskConfig task_config;
    static LoggerConfig logger_config;

    static Logger::LogLevel parseLogLevel(const std::string& level_str);
    static Logger::OutputTarget parseOutputTarget(const std::string& target_str);
};
#define CONF (Config::getInstance())

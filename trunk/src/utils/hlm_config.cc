#include "hlm_config.h"

#include <iostream>
#include <toml++/toml.hpp>

HttpConfig Config::http_config;
TaskConfig Config::task_config;
LoggerConfig Config::logger_config;

Config& Config::getInstance() {
    static Config instance;  // 静态局部变量，线程安全
    return instance;
}

void Config::load(const string& config_file) {
    try {
        toml::table config = toml::parse_file(config_file);
        // 读取Http配置
        auto& http_cfg = *config["http"].as_table();
        http_config.port = http_cfg["port"].value_or(6088);

        // 读取Http配置
        auto& task_cfg = *config["task"].as_table();
        task_config.max_tasks = task_cfg["max_tasks"].value_or(3);

        // 读取日志配置
        auto& logger_cfg = *config["logger"].as_table();
        logger_config.level = logger_cfg["level"].value_or("INFO");
        logger_config.target = logger_cfg["target"].value_or("console");
        logger_config.dir = logger_cfg["dir"].value_or("./logs");
        logger_config.base_name = logger_cfg["base_name"].value_or("hlm");
        logger_config.use_async = logger_cfg["use_async"].value_or(true);
        logger_config.max_file_size = logger_cfg["max_file_size"].value_or(100);
        logger_config.max_files = logger_cfg["max_files"].value_or(5);
    } catch (const toml::parse_error& err) {
        cerr << "Error parsing config file: " << err << "\n";
    }
}

Logger::LogLevel Config::parseLogLevel(const string& level_str) {
    if (level_str == "TRACE") return LogLevel::Trace;
    if (level_str == "DEBUG") return LogLevel::Debug;
    if (level_str == "INFO") return LogLevel::Info;
    if (level_str == "WARN") return LogLevel::Warn;
    if (level_str == "ERROR") return LogLevel::Error;
    return LogLevel::Info;
}

Logger::OutputTarget Config::parseOutputTarget(const string& target_str) {
    if (target_str == "console") return OutputTarget::Console;
    if (target_str == "file") return OutputTarget::File;
    if (target_str == "both") return OutputTarget::Both;
    return OutputTarget::Console;
}

void Config::printAllConfigs() const {
    // 打印Http配置
    hlm_info("Http Configurations: Port: {}", http_config.port);

    // 打印Http配置
    hlm_info("Task Configurations: Max Tasks: {}", task_config.max_tasks);

    // 打印日志配置
    hlm_info("Logger Configurations: Level: {}, Target: {}, Dir: {}, Base Name: {}, Use Async: {}, Max File Size: {}, Max Files: {}",
             logger_config.level, logger_config.target, logger_config.dir, logger_config.base_name, logger_config.use_async,
             logger_config.max_file_size, logger_config.max_files);
}

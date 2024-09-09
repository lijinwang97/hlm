#include "hlm_config.h"

#include <iostream>
#include <toml++/toml.hpp>

FileConfig Config::file_config;
HttpConfig Config::http_config;
LoggerConfig Config::logger_config;
VideoConfig Config::video_config;
AudioConfig Config::audio_config;

Config& Config::getInstance() {
  static Config instance;  // 静态局部变量，线程安全
  return instance;
}

void Config::load(const std::string& config_file) {
  try {
    toml::table config = toml::parse_file(config_file);
    // 读取输入输出文件信息
    auto& file_cfg = *config["files"].as_table();
    file_config.input_file = file_cfg["input_file"].value_or("input.mp4");
    file_config.output_file = file_cfg["output_file"].value_or("output.mp4");

    // 读取Http配置
    auto& http_cfg = *config["http"].as_table();
    http_config.port = http_cfg["port"].value_or(6088);

    // 读取日志配置
    auto& logger_cfg = *config["logger"].as_table();
    logger_config.level = logger_cfg["level"].value_or("INFO");
    logger_config.target = logger_cfg["target"].value_or("console");
    logger_config.base_name = logger_cfg["base_name"].value_or("log");
    logger_config.use_async = logger_cfg["use_async"].value_or(true);
    logger_config.max_file_size = logger_cfg["max_file_size"].value_or(100);
    logger_config.max_files = logger_cfg["max_files"].value_or(5);

    // 读取视频配置
    auto& video_cfg = *config["video"].as_table();
    video_config.codec = video_cfg["codec"].value_or("libx264");
    video_config.bitrate = video_cfg["bitrate"].value_or(3000);
    video_config.width = video_cfg["width"].value_or(1080);
    video_config.height = video_cfg["height"].value_or(1920);
    video_config.framerate = video_cfg["framerate"].value_or(25);
    video_config.gop_size = video_cfg["gop_size"].value_or(60);

    // 读取音频配置
    auto& audio_cfg = *config["audio"].as_table();
    audio_config.codec = audio_cfg["codec"].value_or("aac");
    audio_config.bitrate = audio_cfg["bitrate"].value_or(128);
    audio_config.sample_rate = audio_cfg["sample_rate"].value_or(44100);
    audio_config.channels = audio_cfg["channels"].value_or(2);
  } catch (const toml::parse_error& err) {
    std::cerr << "Error parsing config file: " << err << "\n";
  }
}

Logger::LogLevel Config::parseLogLevel(const std::string& level_str) {
  if (level_str == "TRACE") return LogLevel::Trace;
  if (level_str == "DEBUG") return LogLevel::Debug;
  if (level_str == "INFO") return LogLevel::Info;
  if (level_str == "WARN") return LogLevel::Warn;
  if (level_str == "ERROR") return LogLevel::Error;
  return LogLevel::Info;
}

Logger::OutputTarget Config::parseOutputTarget(const std::string& target_str) {
  if (target_str == "console") return OutputTarget::Console;
  if (target_str == "file") return OutputTarget::File;
  if (target_str == "both") return OutputTarget::Both;
  return OutputTarget::Console;
}

void Config::printAllConfigs() const {
  // 打印文件配置
  hlm_info("File Configurations: Input File: {}, Output File: {}", file_config.input_file, file_config.output_file);

  // 打印Http配置
  hlm_info("Http Configurations: port: {}", http_config.port);

  // 打印日志配置
  hlm_info("Logger Configurations: Level: {}, Target: {}, Base Name: {}, Use Async: {}, Max File Size: {}, Max Files: {}",
           logger_config.level, logger_config.target, logger_config.base_name, logger_config.use_async,
           logger_config.max_file_size, logger_config.max_files);

  // 打印视频配置
  hlm_info("Video Configurations: Codec: {}, Bitrate: {}, Resolution: {}x{}, Framerate: {}, GOP Size: {}",
           video_config.codec, video_config.bitrate, video_config.width, video_config.height,
           video_config.framerate, video_config.gop_size);

  // 打印音频配置
  hlm_info("Audio Configurations: Codec: {}, Bitrate: {}, Sample Rate: {}, Channels: {}",
           audio_config.codec, audio_config.bitrate, audio_config.sample_rate, audio_config.channels);
}

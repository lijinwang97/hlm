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

struct LoggerConfig {
  std::string level;
  std::string target;
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

  // File Config Accessors
  const std::string& getInputFile() const { return file_config.input_file; }
  const std::string& getOutputFile() const { return file_config.output_file; }

  // Http Config Accessors
  const int getHttpPort() const { return http_config.port; }

  // Logger Config Accessors
  Logger::LogLevel getLogLevel() const { return parseLogLevel(logger_config.level); }
  Logger::OutputTarget getLogTarget() const { return parseOutputTarget(logger_config.target); }
  const std::string& getLogBaseName() const { return logger_config.base_name; }
  bool useAsyncLogging() const { return logger_config.use_async; }
  std::size_t getLogMaxFileSize() const { return logger_config.max_file_size * 1024 * 1024; }
  std::size_t getLogMaxFiles() const { return logger_config.max_files; }

  // Video Config Accessors
  const std::string& getVideoCodec() const { return video_config.codec; }
  int getVideoBitrate() const { return video_config.bitrate; }
  int getWidth() const { return video_config.width; }
  int getHeight() const { return video_config.height; }
  int getFramerate() const { return video_config.framerate; }
  int getGopSize() const { return video_config.gop_size; }

  // Audio Config Accessors
  const std::string& getAudioCodec() const { return audio_config.codec; }
  int getAudioBitrate() const { return audio_config.bitrate; }
  int getSampleRate() const { return audio_config.sample_rate; }
  int getChannels() const { return audio_config.channels; }

 private:
  using LogLevel = Logger::LogLevel;
  using OutputTarget = Logger::OutputTarget;

  Config() = default;  // 构造函数私有化
  ~Config() = default;
  Config(const Config&) = delete;
  Config& operator=(const Config&) = delete;

  static FileConfig file_config;
  static HttpConfig http_config;
  static LoggerConfig logger_config;
  static VideoConfig video_config;
  static AudioConfig audio_config;

  static Logger::LogLevel parseLogLevel(const std::string& level_str);
  static Logger::OutputTarget parseOutputTarget(const std::string& target_str);
};
#define CONF (Config::getInstance())

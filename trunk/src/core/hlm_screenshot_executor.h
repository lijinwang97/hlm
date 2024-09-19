#ifndef HLM_SCREENSHOT_EXECUTOR_H
#define HLM_SCREENSHOT_EXECUTOR_H

#include <chrono>
#include <mutex>
#include <string>
#include <thread>

#include "hlm_decoder.h"
#include "hlm_encoder.h"

using namespace std;

// 通用的 ScreenshotExecutor 基类
class ScreenshotExecutor {
   public:
    ScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, const string& screenshot_method);
    virtual ~ScreenshotExecutor();

    virtual void checkAndSavePacket(AVPacket* encoded_packet) = 0;

    void execute();
    void stop();
    bool isRunning() const;

   protected:
    bool initScreenshot();
    bool processScreenshotFrames();
    void saveFrameAsImage(AVPacket* encoded_packet);

   private:
    bool ensureDirectoryExists(const std::string& dir_path);
    bool openInputStream();
    bool findVideoStream();
    bool initDecoder();
    bool initEncoder();
    bool initScaler();

    static int interruptCallback(void* ctx);
    void updateStartTime();

   protected:
    string stream_url_;
    string output_dir_;
    string filename_prefix_;
    string screenshot_method_;
    bool running_ = false;
    int frame_count_ = 0;

    AVFormatContext* format_context_ = nullptr;
    HlmDecoder* decoder_ = nullptr;
    HlmEncoder* encoder_ = nullptr;
    AVPacket* encoded_packet_ = nullptr;
    int video_stream_index_ = -1;

    int64_t start_time_ = 0;
    int64_t last_checked_time_ = 0;
    static const int64_t CHECK_INTERVAL = 1000000;  // 1秒检查间隔
    static const int64_t TIMEOUT = 3000000;        // 超时3秒
};

// 按时间间隔截图
class HlmIntervalScreenshotExecutor : public ScreenshotExecutor {
   public:
    HlmIntervalScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int interval, const string& screenshot_method);
    void checkAndSavePacket(AVPacket* encoded_packet) override;

   private:
    int interval_;
    int64_t last_saved_timestamp_ = 0;
};

// 按百分比截图
class HlmPercentageScreenshotExecutor : public ScreenshotExecutor {
   public:
    HlmPercentageScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int percentage, const string& screenshot_method);
    void checkAndSavePacket(AVPacket* encoded_packet) override;

   private:
    int percentage_;
    double last_saved_percentage_;
};

// 立即截图
class HlmImmediateScreenshotExecutor : public ScreenshotExecutor {
   public:
    HlmImmediateScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, const string& screenshot_method);
    void checkAndSavePacket(AVPacket* encoded_packet) override;
};

// 指定时间点截图
class HlmSpecificTimeScreenshotExecutor : public ScreenshotExecutor {
   public:
    HlmSpecificTimeScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int time_second, const string& screenshot_method);
    void checkAndSavePacket(AVPacket* encoded_packet) override;

   private:
    int time_second_;
};

#endif  // HLM_SCREENSHOT_EXECUTOR_H

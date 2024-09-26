#ifndef HLM_SCREENSHOT_EXECUTOR_H
#define HLM_SCREENSHOT_EXECUTOR_H

#include <chrono>
#include <mutex>
#include <string>
#include <thread>

#include "hlm_decoder.h"
#include "hlm_encoder.h"
#include "hlm_executor.h"

using namespace std;

// 通用的 ScreenshotExecutor 基类
class HlmScreenshotExecutor : public HlmExecutor {
   public:
    HlmScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, const string& screenshot_method);
    virtual ~HlmScreenshotExecutor() = default;

    bool init() override;
    bool initOutputFile() override;
    void execute() override;
    void processFrames(AVFrame* frame, int stream_index);

   protected:
    bool initEncoder();
    void savePacketAsImage(AVPacket* encoded_packet);
    void flushDecoder();
    void flushEncoder();

   protected:
    string screenshot_method_;
    int frame_count_ = 0;
};

// 按时间间隔截图
class HlmIntervalScreenshotExecutor : public HlmScreenshotExecutor {
   public:
    HlmIntervalScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int interval, const string& screenshot_method);
    void checkAndSavePacket(AVPacket* encoded_packet,int stream_index) override;

   private:
    int interval_;
    int64_t last_saved_timestamp_ = 0;
};

// 按百分比截图
class HlmPercentageScreenshotExecutor : public HlmScreenshotExecutor {
   public:
    HlmPercentageScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int percentage, const string& screenshot_method);
    void checkAndSavePacket(AVPacket* encoded_packet,int stream_index) override;

   private:
    int percentage_;
    double last_saved_percentage_;
};

// 立即截图
class HlmImmediateScreenshotExecutor : public HlmScreenshotExecutor {
   public:
    HlmImmediateScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, const string& screenshot_method);
    void checkAndSavePacket(AVPacket* encoded_packet,int stream_index) override;
};

// 指定时间点截图
class HlmSpecificTimeScreenshotExecutor : public HlmScreenshotExecutor {
   public:
    HlmSpecificTimeScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int time_second, const string& screenshot_method);
    void checkAndSavePacket(AVPacket* encoded_packet,int stream_index) override;

   private:
    int time_second_;
};

#endif  // HLM_SCREENSHOT_EXECUTOR_H

#ifndef HLM_EXECUTOR_H
#define HLM_EXECUTOR_H

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "hlm_decoder.h"
#include "hlm_encoder.h"

using namespace std;

// 通用的 HlmExecutor 基类
class HlmExecutor {
   public:
    HlmExecutor(const string& stream_url, const string& output_dir, const string& filename, const string& media_method);
    virtual ~HlmExecutor();

    virtual void processFrames(AVFrame* frame) = 0;

    void execute();
    void stop();
    bool isRunning() const;

   protected:
    bool initMedia();

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
    string filename_;
    string media_method_;
    bool running_ = false;

    AVFormatContext* format_context_ = nullptr;
    HlmDecoder* decoder_ = nullptr;
    HlmEncoder* encoder_ = nullptr;
    AVPacket* encoded_packet_ = nullptr;
    int video_stream_index_ = -1;

    int64_t start_time_ = 0;
    int64_t last_checked_time_ = 0;
    static const int64_t CHECK_INTERVAL = 1000000;  // 1秒检查间隔
    static const int64_t TIMEOUT = 3000000;         // 超时3秒
};

#endif  // HLM_EXECUTOR_H
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

namespace HlmScreenshotMethod {
const string Interval = "interval";
const string Percentage = "percentage";
const string Immediate = "immediate";
const string SpecificTime = "specific_time";
}  // namespace HlmScreenshotMethod

namespace HlmRecordingMethod {
const string Hls = "hls";
const string Mp4 = "mp4";
}  // namespace HlmRecordingMethod

enum class MediaType {
    Screenshot,
    Recording,
    Mix
};

// 通用的 HlmExecutor 基类
class HlmExecutor {
   public:
    HlmExecutor(const string& stream_url, const string& output_dir, const string& filename, const string& media_method, MediaType media_type);
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
    bool findStreams();
    bool initDecoder();
    bool initEncoder();
    bool initVideoEncoderForImage();
    bool initVideoEncoderForVideo();
    bool initScaler();
    bool initOutputFile();
    void updateStartTime();
    bool isScreenshot();
    bool isRecording();
    bool isMix();

    static int interruptCallback(void* ctx);

   protected:
    string stream_url_;
    string output_dir_;
    string filename_;
    string media_method_;
    MediaType media_type_;
    bool running_ = false;

    AVFormatContext* input_format_context_ = nullptr;
    AVFormatContext* output_format_context_ = nullptr;
    HlmDecoder* video_decoder_ = nullptr;
    HlmDecoder* audio_decoder_ = nullptr;
    HlmEncoder* video_encoder_ = nullptr;
    HlmEncoder* audio_encoder_ = nullptr;
    AVPacket* encoded_packet_ = nullptr;
    int video_stream_index_ = -1;
    int audio_stream_index_ = -1;

    int64_t start_time_ = 0;
    int64_t last_checked_time_ = 0;
    static const int64_t CHECK_INTERVAL = 1000000;  // 1秒检查间隔
    static const int64_t TIMEOUT = 3000000;         // 超时3秒
};

#endif  // HLM_EXECUTOR_H
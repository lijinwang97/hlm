#ifndef HLM_EXECUTOR_H
#define HLM_EXECUTOR_H

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "hlm_decoder.h"
#include "hlm_encoder.h"
#include "utils/hlm_queue.h"
#include "utils/hlm_thread.h"

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

namespace HlmMixMethod {
const string Mix = "mix";
}  // namespace HlmMixMethod

enum class MediaType {
    Screenshot,
    Recording,
    Mix
};

// 通用的 HlmExecutor 基类
class HlmExecutor {
   public:
   HlmExecutor();
    HlmExecutor(const string& stream_url, const string& output_dir, const string& filename, const string& media_method);
    virtual ~HlmExecutor();

    virtual bool init() = 0;
    virtual bool initOutputFile() = 0;
    virtual void execute() = 0;
    virtual void checkAndSavePacket(AVPacket* encoded_packet, int stream_index) = 0;

    void stop();
    bool isRunning() const;
    bool ensureDirectoryExists(const string& dir_path);
    bool openInputStream();
    bool findStreams();
    bool initDecoder();
    bool initScaler();

    void updateStartTime();

   private:
    static int interruptCallback(void* ctx);

   protected:
    string stream_url_;
    string output_dir_;
    string filename_;
    string media_method_;
    bool running_ = false;

    AVFormatContext* input_format_context_ = nullptr;
    AVFormatContext* output_format_context_ = nullptr;
    HlmDecoder* video_decoder_ = nullptr;
    HlmDecoder* audio_decoder_ = nullptr;
    HlmEncoder* video_encoder_ = nullptr;
    HlmEncoder* audio_encoder_ = nullptr;
    int input_video_stream_index_ = -1;
    int input_audio_stream_index_ = -1;
    int output_video_stream_index_ = -1;
    int output_audio_stream_index_ = -1;

    HlmQueue<AVPacket*> video_queue_;
    HlmQueue<AVPacket*> audio_queue_;

    int64_t start_time_ = 0;
    int64_t last_checked_time_ = 0;
    static const int64_t CHECK_INTERVAL = 1000000;  // 1秒检查间隔
    static const int64_t TIMEOUT = 3000000;         // 超时3秒
};

#endif  // HLM_EXECUTOR_H
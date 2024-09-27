#ifndef HLM_RECORDING_EXECUTOR_H
#define HLM_RECORDING_EXECUTOR_H

#include <chrono>
#include <mutex>
#include <string>
#include <thread>

#include "hlm_decoder.h"
#include "hlm_encoder.h"
#include "hlm_executor.h"

using namespace std;

// 通用的 HlmRecordingExecutor 基类
class HlmRecordingExecutor : public HlmExecutor {
   public:
    HlmRecordingExecutor(const string& stream_url, const string& output_dir, const string& filename, const string& recording_method);
    virtual ~HlmRecordingExecutor() = default;

    bool init() override;
    bool initOutputFile() override;
    void execute() override;

   protected:
    void setHlsSegmentFilename();
    void endRecording();

   protected:
    string recording_method_;
};

// 录制为MP4格式
class HlmMp4RecordingExecutor : public HlmRecordingExecutor {
   public:
    HlmMp4RecordingExecutor(const string& stream_url, const string& output_dir, const string& filename, const string& recording_method);
    void checkAndSavePacket(AVPacket* encoded_packet, int stream_index) override;
};

// 录制为HLS格式
class HlmHlsRecordingExecutor : public HlmRecordingExecutor {
   public:
    HlmHlsRecordingExecutor(const string& stream_url, const string& output_dir, const string& filename, const string& recording_method);
    void checkAndSavePacket(AVPacket* encoded_packet, int stream_index) override;
};

#endif  // HLM_RECORDING_EXECUTOR_H

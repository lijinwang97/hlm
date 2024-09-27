#ifndef HLM_RECORDING_TASK_H
#define HLM_RECORDING_TASK_H

#include <memory>
#include <string>

#include "hlm_recording_executor.h"
#include "hlm_task.h"

// 录像任务
class HlmRecordingTask : public HlmTask {
   public:
    HlmRecordingTask(const string& stream_url, const string& method);

    void execute() override;
    void stop() override;

   protected:
    virtual unique_ptr<HlmRecordingExecutor> createExecutor() = 0;

    unique_ptr<HlmRecordingExecutor> executor_;
};

// MP4 录制任务
class HlmMp4RecordingTask : public HlmRecordingTask {
   public:
    HlmMp4RecordingTask(const string& stream_url, const string& method, const string& output_dir, const string& filename);

   protected:
    unique_ptr<HlmRecordingExecutor> createExecutor() override;

   private:
    string output_dir_;
    string filename_;
};

// HLS 录制任务
class HlmHlsRecordingTask : public HlmRecordingTask {
   public:
    HlmHlsRecordingTask(const string& stream_url, const string& method, const string& output_dir, const string& filename);

   protected:
    unique_ptr<HlmRecordingExecutor> createExecutor() override;

   private:
    string output_dir_;
    string filename_;
};

#endif  // HLM_RECORDING_TASK_H
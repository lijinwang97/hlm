
#include "hlm_recording_task.h"

#include "utils/hlm_logger.h"

HlmRecordingTask::HlmRecordingTask(const string& stream_url, const string& method)
    : HlmTask(TaskType::Recording, stream_url, method) {}

void HlmRecordingTask::execute() {
    if (!executor_) {
        executor_ = createExecutor();
    }
    executor_->execute();
}

void HlmRecordingTask::stop() {
    if (executor_ && executor_->isRunning()) {
        executor_->stop();
    }
}

// MP4 录制任务实现
HlmMp4RecordingTask::HlmMp4RecordingTask(const string& stream_url, const string& method, const string& output_dir, const string& filename, int duration)
    : HlmRecordingTask(stream_url, method), output_dir_(output_dir), filename_(filename), duration_(duration) {}

unique_ptr<HlmRecordingExecutor> HlmMp4RecordingTask::createExecutor() {
    return make_unique<HlmMp4RecordingExecutor>(getStreamUrl(), output_dir_, filename_, HlmRecordingMethod::Mp4);
}

// HLS 录制任务实现
HlmHlsRecordingTask::HlmHlsRecordingTask(const string& stream_url, const string& method, const string& output_dir, const string& filename, int segment_duration)
    : HlmRecordingTask(stream_url, method), output_dir_(output_dir), filename_(filename), segment_duration_(segment_duration) {}

unique_ptr<HlmRecordingExecutor> HlmHlsRecordingTask::createExecutor() {
    return make_unique<HlmHlsRecordingExecutor>(getStreamUrl(), output_dir_, filename_, HlmRecordingMethod::Hls);
}
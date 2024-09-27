#include "hlm_recording_strategy.h"
#include "hlm_recording_task.h"

shared_ptr<HlmTask> HlmMp4RecordingStrategy::createTask(const string& stream_url, const string& method, const string& output_dir, const string& filename, const json::rvalue& body) {
    return make_shared<HlmMp4RecordingTask>(stream_url, method, output_dir, filename);
}

shared_ptr<HlmTask> HlmHlsRecordingStrategy::createTask(const string& stream_url, const string& method, const string& output_dir, const string& filename, const json::rvalue& body) {
    return make_shared<HlmHlsRecordingTask>(stream_url, method, output_dir, filename);
}

shared_ptr<HlmRecordingStrategy> HlmRecordingStrategyFactory::createStrategy(const string& method) {
    if (method == HlmRecordingMethod::Mp4) {
        return make_shared<HlmMp4RecordingStrategy>();
    } else if (method == HlmRecordingMethod::Hls) {
        return make_shared<HlmHlsRecordingStrategy>();
    } else {
        throw invalid_argument("Invalid recording method.");
    }
}

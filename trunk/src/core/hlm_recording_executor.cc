#include "hlm_recording_executor.h"

#include <filesystem>

#include "utils/hlm_logger.h"
#include "utils/hlm_time.h"

HlmRecordingExecutor::HlmRecordingExecutor(const string& stream_url, const string& output_dir, const string& filename, const string& recording_method)
    : HlmExecutor(stream_url, output_dir, filename, recording_method, MediaType::Recording), recording_method_(recording_method) {}

void HlmRecordingExecutor::savePacket(AVPacket* encoded_packet) {
}

void HlmRecordingExecutor::processFrames(AVFrame* frame) {
    encoded_packet_ = av_packet_alloc();
    if (video_encoder_->encodeFrame(frame, encoded_packet_)) {
        checkAndSavePacket(encoded_packet_);
        av_packet_unref(encoded_packet_);
    } else {
        hlm_error("Failed to encode frame.");
    }
}

HlmMp4RecordingExecutor::HlmMp4RecordingExecutor(const string& stream_url, const string& output_dir, const string& filename, const string& recording_method)
    : HlmRecordingExecutor(stream_url, output_dir, filename, recording_method) {
    hlm_info("Initialized MP4 recording for stream: {}", stream_url);
}

void HlmMp4RecordingExecutor::checkAndSavePacket(AVPacket* encoded_packet) {
    double frame_time = encoded_packet->pts * av_q2d(input_format_context_->streams[video_stream_index_]->time_base);
    hlm_debug("Frame time: {}s, pts: {}", frame_time, encoded_packet->pts);
}

HlmHlsRecordingExecutor::HlmHlsRecordingExecutor(const string& stream_url, const string& output_dir, const string& filename, const string& recording_method)
    : HlmRecordingExecutor(stream_url, output_dir, filename, recording_method) {
    hlm_info("Initialized HLS recording for stream: {}", stream_url);
}

void HlmHlsRecordingExecutor::checkAndSavePacket(AVPacket* encoded_packet) {
    double frame_time = encoded_packet->pts * av_q2d(input_format_context_->streams[video_stream_index_]->time_base);
    hlm_debug("Frame time: {}s, pts: {}", frame_time, encoded_packet->pts);
}

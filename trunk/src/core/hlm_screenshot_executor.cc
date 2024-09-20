#include "hlm_screenshot_executor.h"

#include <filesystem>

#include "utils/hlm_logger.h"
#include "utils/hlm_time.h"

// 通用 HlmScreenshotExecutor 基类的实现
HlmScreenshotExecutor::HlmScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, const string& screenshot_method)
    : HlmExecutor(stream_url, output_dir, filename_prefix, screenshot_method, MediaType::Screenshot), screenshot_method_(screenshot_method) {
}

void HlmScreenshotExecutor::processFrames(AVFrame* frame) {
    AVFrame* scaled_frame = video_encoder_->scaleFrame(frame);
    if (!scaled_frame) {
        hlm_error("Failed to scale frame for saving.");
        return;
    }

    encoded_packet_ = av_packet_alloc();
    if (video_encoder_->encodeFrame(scaled_frame, encoded_packet_)) {
        checkAndSavePacket(encoded_packet_);
        av_packet_unref(encoded_packet_);
    } else {
        hlm_error("Failed to encode frame.");
    }
}

void HlmScreenshotExecutor::savePacketAsImage(AVPacket* encoded_packet) {
    string output_filename = output_dir_ + "/" + filename_ + "_" + to_string(frame_count_) + ".png";
    FILE* file = fopen(output_filename.c_str(), "wb");

    if (file) {
        fwrite(encoded_packet->data, 1, encoded_packet->size, file);
        fclose(file);
        hlm_info("Frame successfully saved to: {}", output_filename);
    } else {
        hlm_error("Failed to open file for saving frame: {}", output_filename);
    }

    frame_count_++;
}

// 按时间间隔截图的实现
HlmIntervalScreenshotExecutor::HlmIntervalScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int interval, const string& screenshot_method)
    : HlmScreenshotExecutor(stream_url, output_dir, filename_prefix, screenshot_method), interval_(interval) {}

void HlmIntervalScreenshotExecutor::checkAndSavePacket(AVPacket* encoded_packet) {
    double frame_time = encoded_packet->pts * av_q2d(input_format_context_->streams[video_stream_index_]->time_base);
    hlm_debug("Frame time: {}s, last saved time: {}s, interval: {}s", frame_time, last_saved_timestamp_, interval_);

    if (frame_time - last_saved_timestamp_ >= interval_) {
        savePacketAsImage(encoded_packet);
        last_saved_timestamp_ = frame_time;
        hlm_info("Saving frame at time: {}s (interval: {}).", frame_time, interval_);
    }
}

// 按百分比截图的实现
HlmPercentageScreenshotExecutor::HlmPercentageScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int percentage, const string& screenshot_method)
    : HlmScreenshotExecutor(stream_url, output_dir, filename_prefix, screenshot_method), percentage_(percentage), last_saved_percentage_(0) {}

void HlmPercentageScreenshotExecutor::checkAndSavePacket(AVPacket* encoded_packet) {
    // 实际视频时长可能会小于显示视频时长，导致达不到100%而少一张图片，后续解决
    double frame_time = encoded_packet->pts * av_q2d(input_format_context_->streams[video_stream_index_]->time_base);
    double total_duration = input_format_context_->duration / AV_TIME_BASE;
    double current_percentage = (frame_time / total_duration) * 100;
    hlm_debug("Current percentage: {}%, last saved percentage: {}%, target percentage: {}%, frame_time:{}s, total_duration:{}",
              current_percentage, last_saved_percentage_, percentage_, frame_time, total_duration);

    if (current_percentage - last_saved_percentage_ >= percentage_) {
        savePacketAsImage(encoded_packet);
        last_saved_percentage_ = current_percentage;
        hlm_info("Saving frame at percentage: {}% (interval: {}%).", current_percentage, percentage_);
    }
}

// 立即截图的实现
HlmImmediateScreenshotExecutor::HlmImmediateScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, const string& screenshot_method)
    : HlmScreenshotExecutor(stream_url, output_dir, filename_prefix, screenshot_method) {}

void HlmImmediateScreenshotExecutor::checkAndSavePacket(AVPacket* encoded_packet) {
    savePacketAsImage(encoded_packet);
    hlm_info("Saving frame at immediate.");
    stop();
}

// 指定时间点截图的实现
HlmSpecificTimeScreenshotExecutor::HlmSpecificTimeScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int time_second, const string& screenshot_method)
    : HlmScreenshotExecutor(stream_url, output_dir, filename_prefix, screenshot_method), time_second_(time_second) {}

void HlmSpecificTimeScreenshotExecutor::checkAndSavePacket(AVPacket* encoded_packet) {
    double frame_time = encoded_packet->pts * av_q2d(input_format_context_->streams[video_stream_index_]->time_base);
    hlm_debug("Frame time: {}s, target time: {}s", frame_time, time_second_);

    if (frame_time >= time_second_) {
        savePacketAsImage(encoded_packet);
        hlm_info("Saving frame at time: {}s (target time: {}s).", frame_time, time_second_);
        stop();
    }
}

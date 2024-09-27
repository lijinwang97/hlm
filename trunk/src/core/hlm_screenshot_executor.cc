#include "hlm_screenshot_executor.h"

#include <filesystem>

#include "utils/hlm_logger.h"
#include "utils/hlm_time.h"

// 通用 HlmScreenshotExecutor 基类的实现
HlmScreenshotExecutor::HlmScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, const string& screenshot_method)
    : HlmExecutor(stream_url, output_dir, filename_prefix, screenshot_method), screenshot_method_(screenshot_method) {
}

bool HlmScreenshotExecutor::init() {
    if (!ensureDirectoryExists(output_dir_)) {
        hlm_error("Failed to create or access output directory: {}", output_dir_);
        return false;
    }

    if (!openInputStream()) {
        hlm_error("Failed to open input stream: {}", stream_url_);
        return false;
    }

    if (!findStreams()) {
        hlm_error("Failed to find video stream in input.");
        return false;
    }

    if (!initDecoder()) {
        hlm_error("Failed to initialize video decoder.");
        return false;
    }

    if (!initEncoder()) {
        hlm_error("Failed to initialize video encoder for image.");
        return false;
    }

    if (!initScaler()) {
        hlm_error("Failed to initialize scaler for image.");
        return false;
    }

    hlm_info("Media initialization successful for screenshot.");

    running_ = true;
    return true;
}

bool HlmScreenshotExecutor::initOutputFile() {
    if (avformat_alloc_output_context2(&output_format_context_, nullptr, nullptr, filename_.c_str()) < 0) {
        hlm_error("Failed to allocate output format context.");
        return false;
    }

    if (!(output_format_context_->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_format_context_->pb, filename_.c_str(), AVIO_FLAG_WRITE) < 0) {
            hlm_error("Failed to open output file: {}", filename_);
            return false;
        }
    }

    hlm_info("Output file initialized: {}", filename_);
    return true;
}

bool HlmScreenshotExecutor::initEncoder() {
    if (input_video_stream_index_ != -1) {
        video_encoder_ = new HlmEncoder();
        if (!video_encoder_->initEncoderForImage(video_decoder_->getContext(), "png")) {
            hlm_error("Failed to initialize video encoder for image.");
            return false;
        }
        return true;
    }
    return false;
}

void HlmScreenshotExecutor::execute() {
    hlm_info("Starting {} for stream: {}", media_method_, stream_url_);
    if (!init()) {
        hlm_error("{} initialization failed for stream: {}", media_method_, stream_url_);
        return;
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    while (isRunning() && av_read_frame(input_format_context_, packet) >= 0) {
        updateStartTime();

        if (packet->stream_index == input_video_stream_index_) {
            if (video_decoder_->decodePacket(packet, frame)) {
                processFrames(frame, packet->stream_index);
            }
        }
        av_packet_unref(packet);
    }

    flushDecoder();
    flushEncoder();
    av_frame_free(&frame);
    av_packet_free(&packet);
    hlm_info("{} stopped for stream: {}", media_method_, stream_url_);
}

void HlmScreenshotExecutor::processFrames(AVFrame* frame, int stream_index) {
    int64_t pts = frame->pts;
    double frame_time = pts * av_q2d(input_format_context_->streams[input_video_stream_index_]->time_base);
    hlm_debug("Processing {} screenshot. PTS: {}, time: {}s, key frame: {}", screenshot_method_, pts, frame_time, frame->key_frame);

    AVFrame* scaled_frame = video_encoder_->scaleFrame(frame);
    if (!scaled_frame) {
        hlm_error("Failed to scale frame for saving.");
        return;
    }

    AVPacket* encoded_packet = av_packet_alloc();
    if (video_encoder_->encodeFrame(scaled_frame, encoded_packet)) {
        checkAndSavePacket(encoded_packet, input_video_stream_index_);
        av_packet_unref(encoded_packet);
    } else {
        hlm_error("Failed to encode frame.");
    }
    av_packet_free(&encoded_packet);
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

void HlmScreenshotExecutor::flushDecoder() {
    hlm_info("Flushing decoder...");
    if (video_decoder_) {
        video_decoder_->flushDecoder([this](AVFrame* frame, int stream_index) {
            updateStartTime();
            processFrames(frame, stream_index);
        });
    }

    if (audio_decoder_) {
        audio_decoder_->flushDecoder([this](AVFrame* frame, int stream_index) {
            updateStartTime();
            processFrames(frame, stream_index);
        });
    }
    hlm_info("Decoder flush completed.");
}

void HlmScreenshotExecutor::flushEncoder() {
    hlm_info("Flushing encoders...");

    if (video_encoder_) {
        video_encoder_->flushEncoder([this](AVPacket* packet, int stream_index) {
            updateStartTime();
            checkAndSavePacket(packet, stream_index);
        });
    }

    if (audio_encoder_) {
        audio_encoder_->flushEncoder([this](AVPacket* packet, int stream_index) {
            updateStartTime();
            checkAndSavePacket(packet, stream_index);
        });
    }

    hlm_info("Encoder flush completed.");
}

// 按时间间隔截图的实现
HlmIntervalScreenshotExecutor::HlmIntervalScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int interval, const string& screenshot_method)
    : HlmScreenshotExecutor(stream_url, output_dir, filename_prefix, screenshot_method), interval_(interval) {}

void HlmIntervalScreenshotExecutor::checkAndSavePacket(AVPacket* encoded_packet, int stream_index) {
    double frame_time = encoded_packet->pts * av_q2d(input_format_context_->streams[input_video_stream_index_]->time_base);
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

void HlmPercentageScreenshotExecutor::checkAndSavePacket(AVPacket* encoded_packet, int stream_index) {
    // 实际视频时长可能会小于显示视频时长，导致达不到100%而少一张图片，后续解决
    double frame_time = encoded_packet->pts * av_q2d(input_format_context_->streams[input_video_stream_index_]->time_base);
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

void HlmImmediateScreenshotExecutor::checkAndSavePacket(AVPacket* encoded_packet, int stream_index) {
    savePacketAsImage(encoded_packet);
    hlm_info("Saving frame at immediate.");
    stop();
}

// 指定时间点截图的实现
HlmSpecificTimeScreenshotExecutor::HlmSpecificTimeScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int time_second, const string& screenshot_method)
    : HlmScreenshotExecutor(stream_url, output_dir, filename_prefix, screenshot_method), time_second_(time_second) {}

void HlmSpecificTimeScreenshotExecutor::checkAndSavePacket(AVPacket* encoded_packet, int stream_index) {
    double frame_time = encoded_packet->pts * av_q2d(input_format_context_->streams[input_video_stream_index_]->time_base);
    hlm_debug("Frame time: {}s, target time: {}s", frame_time, time_second_);

    if (frame_time >= time_second_) {
        savePacketAsImage(encoded_packet);
        hlm_info("Saving frame at time: {}s (target time: {}s).", frame_time, time_second_);
        stop();
    }
}

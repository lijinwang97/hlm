#include "hlm_screenshot_executor.h"

#include <filesystem>

#include "utils/hlm_logger.h"

// 通用 ScreenshotExecutor 基类的实现
ScreenshotExecutor::ScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix)
    : stream_url_(stream_url), output_dir_(output_dir), filename_prefix_(filename_prefix) {
    encoded_packet_ = av_packet_alloc();
    scaled_frame_ = av_frame_alloc();
}

ScreenshotExecutor::~ScreenshotExecutor() {
    if (encoded_packet_) {
        av_packet_free(&encoded_packet_);
        encoded_packet_ = nullptr;
    }

    if (scaled_frame_) {
        av_frame_free(&scaled_frame_);
        scaled_frame_ = nullptr;
    }

    if (format_context_) {
        avformat_close_input(&format_context_);  // 释放格式上下文
        format_context_ = nullptr;
    }

    if (decoder_) {
        delete decoder_;
        decoder_ = nullptr;
    }

    if (encoder_) {
        delete encoder_;
        encoder_ = nullptr;
    }
}

void ScreenshotExecutor::execute() {
    initScreenshot();
    hlm_info("Starting screenshot capture for stream: {}", stream_url_);

    processScreenshotFrames();
    hlm_info("Screenshot capture stopped for stream: {}", stream_url_);
}

void ScreenshotExecutor::stop() {
    running_ = false;
    hlm_info("Stopping screenshot capture for stream: {}", stream_url_);
}

void ScreenshotExecutor::initScreenshot() {
    if (!ensureDirectoryExists(output_dir_)) {
        return;
    }

    if (!openInputStream()) {
        hlm_error("Failed to open input stream.");
        return;
    }
    if (!findVideoStream()) {
        hlm_error("Failed to find video stream.");
        return;
    }
    if (!initDecoder()) {
        hlm_error("Failed to initialize decoder.");
        return;
    }

    if (!initEncoder()) {
        hlm_error("Failed to initialize encoder.");
        return;
    }

    if (!initScaler()) {
        hlm_error("Failed to initialize scaler.");
        return;
    }

    running_ = true;
}

bool ScreenshotExecutor::processScreenshotFrames() {
    AVPacket packet;
    AVFrame* frame = av_frame_alloc();

    while (running_ && av_read_frame(format_context_, &packet) >= 0) {
        if (packet.stream_index == video_stream_index_) {
            if (decoder_->decodePacket(&packet, frame)) {
                scaled_frame_ = encoder_->scaleFrame(frame);
                if (!scaled_frame_) {
                    hlm_error("Failed to scale frame for saving.");
                    continue;
                }

                encoded_packet_ = av_packet_alloc();
                if (encoder_->encodeFrame(scaled_frame_, encoded_packet_)) {
                    checkAndSavePacket(encoded_packet_);
                    av_packet_unref(encoded_packet_);
                } else {
                    hlm_error("Failed to encode frame.");
                }
            }
        }
        av_packet_unref(&packet);
    }

    av_frame_free(&frame);
    return true;
}

void ScreenshotExecutor::saveFrameAsImage(AVPacket* encoded_packet) {
    std::string output_filename = output_dir_ + "/" + filename_prefix_ + "_" + std::to_string(frame_count_) + ".png";
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

bool ScreenshotExecutor::ensureDirectoryExists(const std::string& dir_path) {
    if (!std::filesystem::exists(dir_path)) {
        hlm_info("Output directory does not exist. Creating: {}", dir_path);
        try {
            std::filesystem::create_directories(dir_path);
            hlm_info("Output directory created successfully: {}", dir_path);
            return true;
        } catch (const std::filesystem::filesystem_error& e) {
            hlm_error("Failed to create output directory: {}. Error: {}", dir_path, e.what());
            return false;
        }
    }
    return true;
}

bool ScreenshotExecutor::openInputStream() {
    if (avformat_open_input(&format_context_, stream_url_.c_str(), nullptr, nullptr) != 0) {
        hlm_error("Failed to open stream: {}", stream_url_);
        return false;
    }
    if (avformat_find_stream_info(format_context_, nullptr) < 0) {
        hlm_error("Failed to retrieve stream info for: {}", stream_url_);
        return false;
    }
    return true;
}

bool ScreenshotExecutor::findVideoStream() {
    for (unsigned int i = 0; i < format_context_->nb_streams; i++) {
        if (format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index_ = i;
            return true;
        }
    }
    return false;
}

bool ScreenshotExecutor::initDecoder() {
    decoder_ = new HlmDecoder(video_stream_index_);
    if (!decoder_->initDecoder(format_context_)) {
        hlm_error("Failed to initialize decoder for stream: {}", stream_url_);
        return false;
    }
    return true;
}

bool ScreenshotExecutor::initEncoder() {
    encoder_ = new HlmEncoder();
    if (!encoder_->initEncoder(format_context_, decoder_->getContext(), "png")) {
        hlm_error("Failed to initialize encoder.");
        return false;
    }
    return true;
}

bool ScreenshotExecutor::initScaler() {
    if (!encoder_) {
        encoder_ = new HlmEncoder();
    }
    if (!encoder_->initScaler(decoder_->getContext()->width, decoder_->getContext()->height, decoder_->getContext()->pix_fmt,
                              decoder_->getContext()->width, decoder_->getContext()->height, AV_PIX_FMT_RGB24)) {
        hlm_error("Failed to initialize scaler.");
        return false;
    }
    return true;
}

// 按时间间隔截图的实现
HlmIntervalScreenshotExecutor::HlmIntervalScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int interval)
    : ScreenshotExecutor(stream_url, output_dir, filename_prefix), interval_(interval) {}

void HlmIntervalScreenshotExecutor::checkAndSavePacket(AVPacket* encoded_packet) {
    int64_t pts = encoded_packet->pts;
    double frame_time = pts * av_q2d(format_context_->streams[video_stream_index_]->time_base);

    hlm_debug("Encoded frame. PTS: {}, frame time: {}s, last saved time: {}s",
              pts, frame_time, last_saved_timestamp_);

    if (frame_time - last_saved_timestamp_ >= interval_) {
        saveFrameAsImage(encoded_packet);
        last_saved_timestamp_ = frame_time;
        hlm_info("Saving frame at time: {}s (interval: {}).", frame_time, interval_);
    }
}

// 按百分比截图的实现
HlmPercentageScreenshotExecutor::HlmPercentageScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int percentage)
    : ScreenshotExecutor(stream_url, output_dir, filename_prefix), percentage_(percentage) {}

void HlmPercentageScreenshotExecutor::checkAndSavePacket(AVPacket* encoded_packet) {
}

// 立即截图的实现
HlmImmediateScreenshotExecutor::HlmImmediateScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix)
    : ScreenshotExecutor(stream_url, output_dir, filename_prefix) {}

void HlmImmediateScreenshotExecutor::checkAndSavePacket(AVPacket* encoded_packet) {
}

// 指定时间点截图的实现
HlmSpecificTimeScreenshotExecutor::HlmSpecificTimeScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int time_second)
    : ScreenshotExecutor(stream_url, output_dir, filename_prefix), time_second_(time_second) {}

void HlmSpecificTimeScreenshotExecutor::checkAndSavePacket(AVPacket* encoded_packet) {
}

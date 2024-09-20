#include "hlm_executor.h"

#include "utils/hlm_logger.h"
#include "utils/hlm_time.h"

#include <filesystem>

HlmExecutor::HlmExecutor(const string& stream_url, const string& output_dir, const string& filename, const string& media_method)
    : stream_url_(stream_url), output_dir_(output_dir), filename_(filename), media_method_(media_method) {}

HlmExecutor::~HlmExecutor() {
    if (encoded_packet_) {
        av_packet_free(&encoded_packet_);
        encoded_packet_ = nullptr;
    }

    if (format_context_) {
        avformat_close_input(&format_context_);
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

void HlmExecutor::execute() {
    hlm_info("Starting {} for stream: {}", media_method_, stream_url_);
    if (!initMedia()) {
        hlm_error("{} initialization failed for stream: {}", media_method_, stream_url_);
        return;
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    while (isRunning() && av_read_frame(format_context_, packet) >= 0) {
        updateStartTime();
        if (packet->stream_index == video_stream_index_) {
            if (decoder_->decodePacket(packet, frame)) {
                int64_t pts = frame->pts;
                double frame_time = pts * av_q2d(format_context_->streams[video_stream_index_]->time_base);
                hlm_debug("Processing {}. PTS: {}, time: {}s, key frame: {}", media_method_, pts, frame_time, frame->key_frame);

                processFrames(frame);
            }
        }
        av_packet_unref(packet);
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
    hlm_info("{} stopped for stream: {}", media_method_, stream_url_);
}

void HlmExecutor::stop() {
    running_ = false;
}

bool HlmExecutor::isRunning() const {
    return running_;
}

bool HlmExecutor::initMedia() {
    if (!ensureDirectoryExists(output_dir_)) {
        return false;
    }

    if (!openInputStream()) {
        return false;
    }

    if (!findVideoStream()) {
        hlm_error("Failed to find video stream.");
        return false;
    }
    if (!initDecoder()) {
        hlm_error("Failed to initialize decoder.");
        return false;
    }

    if (!initEncoder()) {
        hlm_error("Failed to initialize encoder.");
        return false;
    }

    if (!initScaler()) {
        hlm_error("Failed to initialize scaler.");
        return false;
    }

    running_ = true;
    return true;
}

bool HlmExecutor::ensureDirectoryExists(const string& dir_path) {
    if (!filesystem::exists(dir_path)) {
        hlm_info("Output directory does not exist. Creating: {}", dir_path);
        try {
            filesystem::create_directories(dir_path);
            hlm_info("Output directory created successfully: {}", dir_path);
            return true;
        } catch (const filesystem::filesystem_error& e) {
            hlm_error("Failed to create output directory: {}. Error: {}", dir_path, e.what());
            return false;
        }
    }
    return true;
}

bool HlmExecutor::openInputStream() {
    format_context_ = avformat_alloc_context();
    if (!format_context_) {
        hlm_error("Failed to allocate AVFormatContext.");
        return false;
    }

    // 设置超时回调，用于处理流异常及读取到流结束进行自动退出
    updateStartTime();
    AVIOInterruptCB interrupt_cb = {interruptCallback, this};
    format_context_->interrupt_callback = interrupt_cb;

    if (avformat_open_input(&format_context_, stream_url_.c_str(), nullptr, nullptr) != 0) {
        hlm_error("Failed to open stream: {}", stream_url_);
        return false;
    }
    updateStartTime();

    if (avformat_find_stream_info(format_context_, nullptr) < 0) {
        hlm_error("Failed to retrieve stream info for: {}", stream_url_);
        return false;
    }
    return true;
}

bool HlmExecutor::findVideoStream() {
    for (unsigned int i = 0; i < format_context_->nb_streams; i++) {
        if (format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index_ = i;
            return true;
        }
    }
    return false;
}

bool HlmExecutor::initDecoder() {
    decoder_ = new HlmDecoder(video_stream_index_);
    if (!decoder_->initDecoder(format_context_)) {
        hlm_error("Failed to initialize decoder for stream: {}", stream_url_);
        return false;
    }
    return true;
}

bool HlmExecutor::initEncoder() {
    encoder_ = new HlmEncoder();
    if (!encoder_->initEncoderForImage(decoder_->getContext(), "png")) {
        hlm_error("Failed to initialize encoder.");
        return false;
    }
    return true;
}

bool HlmExecutor::initScaler() {
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

int HlmExecutor::interruptCallback(void* ctx) {
    HlmExecutor* executor = static_cast<HlmExecutor*>(ctx);

    int64_t current_time = getCurrentTimeInMicroseconds();
    int64_t elapsed_time = current_time - executor->last_checked_time_;

    if (elapsed_time > CHECK_INTERVAL) {
        executor->last_checked_time_ = current_time;
        int64_t total_elapsed_time = current_time - executor->start_time_;
        hlm_debug("Interrupt callback invoked: Total elapsed time: {}µs, Start time: {}µs, Current time: {}µs",
                  total_elapsed_time, executor->start_time_, current_time);
        if (total_elapsed_time > TIMEOUT) {
            hlm_error("Timeout reached for stream: {}", executor->stream_url_);
            return 1;
        }
    }

    return 0;
}

void HlmExecutor::updateStartTime() {
    start_time_ = getCurrentTimeInMicroseconds();
}
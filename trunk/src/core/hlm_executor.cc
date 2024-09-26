#include "hlm_executor.h"

#include <filesystem>

#include "utils/hlm_logger.h"
#include "utils/hlm_time.h"

HlmExecutor::HlmExecutor(const string& stream_url, const string& output_dir, const string& filename, const string& media_method, MediaType media_type)
    : stream_url_(stream_url), output_dir_(output_dir), filename_(filename), media_method_(media_method), media_type_(media_type) {     
}

HlmExecutor::~HlmExecutor() {
    if (video_encoder_) {
        delete video_encoder_;
        video_encoder_ = nullptr;
    }

    if (video_decoder_) {
        delete video_decoder_;
        video_decoder_ = nullptr;
    }

    if (audio_encoder_) {
        delete audio_encoder_;
        audio_encoder_ = nullptr;
    }

    if (audio_decoder_) {
        delete audio_decoder_;
        audio_decoder_ = nullptr;
    }

    if (input_format_context_) {
        avformat_close_input(&input_format_context_);
        input_format_context_ = nullptr;
    }
}

void HlmExecutor::stop() {
    running_ = false;
}

bool HlmExecutor::isRunning() const {
    return running_;
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
    av_log_set_level(AV_LOG_ERROR);

    input_format_context_ = avformat_alloc_context();
    if (!input_format_context_) {
        hlm_error("Failed to allocate AVFormatContext.");
        return false;
    }

    // 设置超时回调，用于处理流异常及读取到流结束进行自动退出
    updateStartTime();
    AVIOInterruptCB interrupt_cb = {interruptCallback, this};
    input_format_context_->interrupt_callback = interrupt_cb;

    if (avformat_open_input(&input_format_context_, stream_url_.c_str(), nullptr, nullptr) != 0) {
        hlm_error("Failed to open stream: {}", stream_url_);
        return false;
    }
    updateStartTime();

    if (avformat_find_stream_info(input_format_context_, nullptr) < 0) {
        hlm_error("Failed to retrieve stream info for: {}", stream_url_);
        return false;
    }
    return true;
}

bool HlmExecutor::findStreams() {
    for (unsigned int i = 0; i < input_format_context_->nb_streams; i++) {
        AVCodecParameters* codecpar = input_format_context_->streams[i]->codecpar;
        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO && input_video_stream_index_ == -1) {
            input_video_stream_index_ = i;
        } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO && input_audio_stream_index_ == -1) {
            input_audio_stream_index_ = i;
        }

        if (input_video_stream_index_ != -1 && input_audio_stream_index_ != -1) {
            break;
        }
    }

    if (input_video_stream_index_ != -1) {
        return true;
    }

    hlm_error("No video stream found for stream: {}", stream_url_);
    return false;
}

bool HlmExecutor::initDecoder() {
    bool decoder_initialized = false;

    if (input_video_stream_index_ != -1) {
        video_decoder_ = new HlmDecoder(input_video_stream_index_);
        if (!video_decoder_->initDecoder(input_format_context_)) {
            hlm_error("Failed to initialize video decoder for stream: {}, stream index: {}", stream_url_, input_video_stream_index_);
            return false;
        }
        decoder_initialized = true;
        hlm_info("Video decoder initialized successfully for stream index: {}", input_video_stream_index_);
    }

    if (input_audio_stream_index_ != -1) {
        audio_decoder_ = new HlmDecoder(input_audio_stream_index_);
        if (!audio_decoder_->initDecoder(input_format_context_)) {
            hlm_error("Failed to initialize audio decoder for stream: {}, stream index: {}", stream_url_, input_audio_stream_index_);
            return false;
        }
        decoder_initialized = true;
        hlm_info("Audio decoder initialized successfully for stream index: {}", input_audio_stream_index_);
    }

    if (!decoder_initialized) {
        hlm_error("No decoders initialized for stream: {}", stream_url_);
        return false;
    }

    return true;
}

bool HlmExecutor::initScaler() {
    if (!video_encoder_) {
        video_encoder_ = new HlmEncoder();
    }
    if (!video_encoder_->initScaler(video_decoder_->getContext()->width, video_decoder_->getContext()->height, video_decoder_->getContext()->pix_fmt,
                                    video_decoder_->getContext()->width, video_decoder_->getContext()->height, AV_PIX_FMT_RGB24)) {
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
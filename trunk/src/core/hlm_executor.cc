#include "hlm_executor.h"

#include <filesystem>

#include "utils/hlm_logger.h"
#include "utils/hlm_time.h"

HlmExecutor::HlmExecutor(const string& stream_url, const string& output_dir, const string& filename, const string& media_method, MediaType media_type)
    : stream_url_(stream_url), output_dir_(output_dir), filename_(filename), media_method_(media_method), media_type_(media_type) {}

HlmExecutor::~HlmExecutor() {
    if (video_encoder_) {
        delete video_encoder_;  // 让 HlmEncoder 析构函数自行管理上下文的释放
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

    if (encoded_packet_) {
        av_packet_free(&encoded_packet_);
        encoded_packet_ = nullptr;
    }

    if (input_format_context_) {
        avformat_close_input(&input_format_context_);
        input_format_context_ = nullptr;
    }
}

void HlmExecutor::execute() {
    hlm_info("Starting {} for stream: {}", media_method_, stream_url_);
    if (!initMedia()) {
        hlm_error("{} initialization failed for stream: {}", media_method_, stream_url_);
        return;
    }

    hlm_info("{} stopped for stream: {}", media_method_, stream_url_);
    return;

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    while (isRunning() && av_read_frame(input_format_context_, packet) >= 0) {
        updateStartTime();
        if (packet->stream_index == video_stream_index_) {
            if (video_decoder_->decodePacket(packet, frame)) {
                int64_t pts = frame->pts;
                double frame_time = pts * av_q2d(input_format_context_->streams[video_stream_index_]->time_base);
                hlm_debug("Processing {}. PTS: {}, time: {}s, key frame: {}", media_method_, pts, frame_time, frame->key_frame);

                // processFrames(frame);
            }
        }

        if (isRecording() && packet->stream_index == audio_stream_index_) {
            if (audio_decoder_->decodePacket(packet, frame)) {
                int64_t pts = frame->pts;
                double frame_time = pts * av_q2d(input_format_context_->streams[video_stream_index_]->time_base);
                hlm_debug("Processing audio for {}. PTS: {}, time: {}s", media_method_, frame->pts, frame_time);
                // processFrames(frame);
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

    if (!findStreams()) {
        hlm_error("Failed to find video/audio stream.");
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

    if (isScreenshot() && !initScaler()) {
        hlm_error("Failed to initialize scaler.");
        return false;
    }

    if (!isScreenshot() && !initOutputFile()) {
        hlm_error("Failed to initialize output file.");
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
        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index_ == -1) {
            video_stream_index_ = i;
        } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index_ == -1) {
            audio_stream_index_ = i;
        }

        if (video_stream_index_ != -1 && audio_stream_index_ != -1) {
            break;
        }
    }

    if (video_stream_index_ != -1) {
        return true;
    }

    hlm_error("No video stream found for stream: {}", stream_url_);
    return false;
}

bool HlmExecutor::initDecoder() {
    bool decoder_initialized = false;

    if (video_stream_index_ != -1) {
        video_decoder_ = new HlmDecoder(video_stream_index_);
        if (!video_decoder_->initDecoder(input_format_context_)) {
            hlm_error("Failed to initialize video decoder for stream: {}, stream index: {}", stream_url_, video_stream_index_);
            return false;
        }
        decoder_initialized = true;
        hlm_info("Video decoder initialized successfully for stream index: {}", video_stream_index_);
    }

    if (audio_stream_index_ != -1) {
        audio_decoder_ = new HlmDecoder(audio_stream_index_);
        if (!audio_decoder_->initDecoder(input_format_context_)) {
            hlm_error("Failed to initialize audio decoder for stream: {}, stream index: {}", stream_url_, audio_stream_index_);
            return false;
        }
        decoder_initialized = true;
        hlm_info("Audio decoder initialized successfully for stream index: {}", audio_stream_index_);
    }

    if (!decoder_initialized) {
        hlm_error("No decoders initialized for stream: {}", stream_url_);
        return false;
    }

    return true;
}

bool HlmExecutor::initEncoder() {
    bool encoder_initialized = false;

    if (isScreenshot()) {
        encoder_initialized = initVideoEncoderForImage();
    }

    if (isRecording()) {
        encoder_initialized = initVideoEncoderForVideo();
    }

    if (!encoder_initialized) {
        hlm_error("No encoders initialized for stream: {}", stream_url_);
        return false;
    }

    return true;
}

bool HlmExecutor::initVideoEncoderForImage() {
    video_encoder_ = new HlmEncoder();
    if (!video_encoder_->initEncoderForImage(video_decoder_->getContext(), "png")) {
        hlm_error("Failed to initialize video encoder for image.");
        return false;
    }
    return true;
}

bool HlmExecutor::initVideoEncoderForVideo() {
    bool encoder_initialized = false;

    if (!video_encoder_) {
        video_encoder_ = new HlmEncoder();
        if (!video_encoder_->initEncoderForVideo(video_decoder_->getContext())) {
            hlm_error("Failed to initialize video encoder for recording.");
            return false;
        }
        encoder_initialized = true;
    }

    if (audio_stream_index_ != -1 && !audio_encoder_) {
        audio_encoder_ = new HlmEncoder();
        if (!audio_encoder_->initEncoderForAudio(audio_decoder_->getContext())) {
            hlm_error("Failed to initialize audio encoder for recording.");
            return false;
        }
        encoder_initialized = true;
    } else if (audio_stream_index_ == -1) {
        hlm_warn("No audio stream found for stream: {}", stream_url_);
    }

    if (!encoder_initialized) {
        hlm_error("No encoders initialized for stream: {}", stream_url_);
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

bool HlmExecutor::initOutputFile() {
    if (avformat_alloc_output_context2(&output_format_context_, nullptr, nullptr, filename_.c_str()) < 0) {
        hlm_error("Failed to allocate output format context.");
        return false;
    }

    if (avformat_new_stream(output_format_context_, video_encoder_->getContext()->codec) == nullptr) {
        hlm_error("Failed to create new video stream in output file.");
        return false;
    }

    if (audio_encoder_ && avformat_new_stream(output_format_context_, audio_encoder_->getContext()->codec) == nullptr) {
        hlm_error("Failed to create new audio stream in output file.");
        return false;
    }

    if (!(output_format_context_->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_format_context_->pb, filename_.c_str(), AVIO_FLAG_WRITE) < 0) {
            hlm_error("Failed to open output file: {}", filename_);
            return false;
        }
    }

    if (avformat_write_header(output_format_context_, nullptr) < 0) {
        hlm_error("Failed to write header to output file.");
        return false;
    }

    hlm_info("Output file initialized: {}", filename_);
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

bool HlmExecutor::isScreenshot() {
    return media_type_ == MediaType::Screenshot;
}

bool HlmExecutor::isRecording() {
    return media_type_ == MediaType::Recording;
}

bool HlmExecutor::isMix() {
    return media_type_ == MediaType::Mix;
}
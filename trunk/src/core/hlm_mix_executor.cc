#include "hlm_mix_executor.h"

#include "utils/hlm_logger.h"

HlmMixExecutor::HlmMixExecutor(const HlmMixTaskParams& params)
    : HlmExecutor(), params_(params) {
}

bool HlmMixExecutor::init() {
    hlm_info("Initializing mix resources for output URL: {}", params_.output_url);

    if (!initOutputFile()) {
        hlm_error("Failed to initialize output file.");
        return false;
    }

    if (!loadBackgroundImage(params_.background_image)) {
        hlm_error("Failed to load background image: {}", params_.background_image);
        return false;
    }

    // for (const auto& stream : params_.streams) {
    //     if (!initDecoder(stream)) {
    //         hlm_error("Failed to initialize decoder for stream: {}", stream.id);
    //         return false;
    //     }
    // }

    // if (!initEncoder()) {
    //     hlm_error("Failed to initialize encoder.");
    //     return false;
    // }

    hlm_info("Mix resources initialized successfully.");
    return true;
}

bool HlmMixExecutor::initOutputFile() {
    hlm_info("Initializing FLV output for RTMP stream: {}", params_.output_url);

    if (avformat_alloc_output_context2(&output_format_context_, nullptr, "flv", params_.output_url.c_str()) < 0) {
        hlm_error("Failed to create FLV output context for URL: {}", params_.output_url);
        return false;
    }

    EncoderParams encoder_params;
    encoder_params.width = params_.resolution.width;
    encoder_params.height = params_.resolution.height;
    encoder_params.fps = 30;
    encoder_params.video_bitrate = 2000000;
    encoder_params.video_encoder_name = "h264_nvenc";
    target_pix_fmt_ = AV_PIX_FMT_YUV420P;
    encoder_params.pix_fmt = target_pix_fmt_;
    if (!video_encoder_->initVideoEncoder(encoder_params, output_format_context_)) {
        return false;
    }

    encoder_params.sample_rate = 44100;
    encoder_params.channels = 2;
    encoder_params.audio_bitrate = 128000;
    encoder_params.audio_encoder_name = "aac";
    if (!audio_encoder_->initAudioEncoder(encoder_params, output_format_context_)) {
        return false;
    }

    if (!(output_format_context_->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_format_context_->pb, params_.output_url.c_str(), AVIO_FLAG_WRITE) < 0) {
            hlm_error("Failed to open RTMP stream: {}", params_.output_url);
            return false;
        }
    }

    if (avformat_write_header(output_format_context_, nullptr) < 0) {
        hlm_error("Failed to write header to output URL: {}", params_.output_url);
        return false;
    }

    hlm_info("FLV output initialized successfully for RTMP stream: {}", params_.output_url);
    return true;
}

void HlmMixExecutor::execute() {
    hlm_info("Starting mixing task with output URL: {}", params_.output_url);

    if (!init()) {
        hlm_error("Failed to initialize mix resources for output URL: {}", params_.output_url);
        return;
    }

    for (const auto& stream : params_.streams) {
        // 对每个流进行解码器初始化或其他必要启动操作
        // 。。。

        // 将启动成功的流添加到 active_streams_
        active_streams_[stream.id] = stream;
        hlm_info("Started processing stream: {}, URL: {}, width:{}, height:{}, x:{}, y:{}, z_index:{}", stream.id, stream.url, stream.width, stream.height, stream.x, stream.y, stream.z_index);
    }

    hlm_info("Mixing task completed with output URL: {}", params_.output_url);
}

bool HlmMixExecutor::loadBackgroundImage(const string& image_url) {
    hlm_info("Loading background image from URL: {}", image_url);

    auto image_decoder = std::make_unique<HlmDecoder>(AVMEDIA_TYPE_VIDEO);
    if (!image_decoder->openInputStream(image_url)) {
        hlm_error("Failed to open image file: {}", image_url);
        return false;
    }

    if (!image_decoder->initDecoder()) {
        hlm_error("Failed to initialize decoder for background image: {}", image_url);
        return false;
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    bool image_loaded = false;
    while (av_read_frame(image_decoder->getFormatContext(), packet) >= 0) {
        if (image_decoder->decodePacket(packet, frame)) {
            hlm_info("Background image successfully decoded: {}x{}, format: {}", frame->width, frame->height, frame->format);
            if (frame->width != params_.resolution.width || frame->height != params_.resolution.height || frame->format != AV_PIX_FMT_YUV420P) {
                hlm_info("Rescaling background image to {}x{}.", params_.resolution.width, params_.resolution.height);

                if (!image_decoder->initScaler(frame->width, frame->height, (AVPixelFormat)frame->format,
                                               params_.resolution.width, params_.resolution.height, target_pix_fmt_)) {
                    hlm_error("Failed to initialize scaler for background image.");
                    break;
                }

                AVFrame* scaled_frame = image_decoder->scaleFrame(frame);
                if (!scaled_frame) {
                    hlm_error("Failed to scale background image.");
                    break;
                }

                background_frame_ = av_frame_clone(scaled_frame);
                image_loaded = true;
            } else {
                background_frame_ = av_frame_clone(frame);
                image_loaded = true;
            }
            break;
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    av_frame_free(&frame);

    if (!image_loaded) {
        hlm_error("Failed to decode background image: {}", image_url);
        return false;
    }

    hlm_info("Background image successfully loaded and processed.");
    return true;
}

void HlmMixExecutor::updateStreams(const HlmMixTaskParams& params) {
    lock_guard<mutex> lock(mix_mutex_);

    unordered_map<string, HlmStreamInfo> new_streams_map;
    for (const auto& stream : params.streams) {
        new_streams_map[stream.id] = stream;
    }

    vector<string> streams_to_remove;
    for (const auto& [stream_id, stream_info] : active_streams_) {
        if (new_streams_map.find(stream_id) == new_streams_map.end()) {
            streams_to_remove.push_back(stream_id);
            hlm_info("Removed stream: {}, URL: {}", stream_id, stream_info.url);
        }
    }
    for (const auto& stream_id : streams_to_remove) {
        active_streams_.erase(stream_id);
    }

    for (const auto& [stream_id, new_stream_info] : new_streams_map) {
        auto existing_stream_it = active_streams_.find(stream_id);
        if (existing_stream_it == active_streams_.end()) {
            active_streams_[stream_id] = new_stream_info;
            hlm_info("Added new stream: {}, URL: {}, width: {}, height: {}, x: {}, y: {}, z_index: {}",
                     new_stream_info.id, new_stream_info.url, new_stream_info.width,
                     new_stream_info.height, new_stream_info.x, new_stream_info.y,
                     new_stream_info.z_index);
        } else {
            auto& existing_stream_info = existing_stream_it->second;
            if (existing_stream_info.width != new_stream_info.width ||
                existing_stream_info.height != new_stream_info.height ||
                existing_stream_info.x != new_stream_info.x ||
                existing_stream_info.y != new_stream_info.y ||
                existing_stream_info.z_index != new_stream_info.z_index) {
                hlm_info(
                    "Updating stream: {}, URL: {}\n"
                    "Before - width: {}, height: {}, x: {}, y: {}, z_index: {}\n"
                    "After  - width: {}, height: {}, x: {}, y: {}, z_index: {}",
                    new_stream_info.id, new_stream_info.url,
                    existing_stream_info.width, existing_stream_info.height, existing_stream_info.x, existing_stream_info.y, existing_stream_info.z_index,
                    new_stream_info.width, new_stream_info.height, new_stream_info.x, new_stream_info.y, new_stream_info.z_index);
                existing_stream_info = new_stream_info;
            }
        }
    }

    hlm_info("Streams updated. Total active streams: {}", active_streams_.size());
}

void HlmMixExecutor::endMixing() {
    hlm_info("Ending mixing task for output URL: {}", params_.output_url);
    // 清理混流任务的资源，如关闭输出文件、释放编码器等
}

// 默认混流执行器构造函数
HlmDefaultMixExecutor::HlmDefaultMixExecutor(const HlmMixTaskParams& params)
    : HlmMixExecutor(params) {}

void HlmDefaultMixExecutor::checkAndSavePacket(AVPacket* encoded_packet, int stream_index) {
    hlm_info("Processing and saving mixed frame for stream index: {}", stream_index);

    // 实现混合后的帧处理逻辑，如编码和保存帧到输出文件
    // frame 参数代表解码后的帧
}

#include "hlm_decoder.h"

#include <iostream>

HlmDecoder::HlmDecoder(int stream_index)
    : codec_context_(nullptr), stream_index_(stream_index) {
}

HlmDecoder::HlmDecoder(AVMediaType media_type) : media_type_(media_type) {}

HlmDecoder::~HlmDecoder() {
    if (codec_context_) {
        avcodec_free_context(&codec_context_);  // 手动释放 codec_context_
    }
}

bool HlmDecoder::openInputStream(const string& input_url) {
    format_context_ = avformat_alloc_context();
    if (!format_context_) {
        hlm_error("Failed to allocate AVFormatContext.");
        return false;
    }

    if (avformat_open_input(&format_context_, input_url.c_str(), nullptr, nullptr) != 0) {
        hlm_error("Failed to open stream: {}", input_url);
        return false;
    }

    if (avformat_find_stream_info(format_context_, nullptr) < 0) {
        hlm_error("Failed to retrieve stream info for: {}", input_url);
        return false;
    }

    for (unsigned int i = 0; i < format_context_->nb_streams; ++i) {
        if (format_context_->streams[i]->codecpar->codec_type == media_type_) {
            stream_index_ = i;
            hlm_info("Found {} stream at index {}", av_get_media_type_string(media_type_), stream_index_);
            return true;
        }
    }

    hlm_error("Failed to find {} stream in {}", av_get_media_type_string(media_type_), input_url);
    return false;
}

bool HlmDecoder::initDecoder() {
    if (stream_index_ == -1) {
        hlm_error("No valid stream found to initialize the decoder.");
        return false;
    }

    AVStream* stream = format_context_->streams[stream_index_];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        hlm_error("Failed to find decoder for stream index: {}", stream_index_);
        return false;
    }

    codec_context_ = avcodec_alloc_context3(codec);
    if (!codec_context_) {
        hlm_error("Failed to allocate decoder context for stream index: {}", stream_index_);
        return false;
    }

    if (avcodec_parameters_to_context(codec_context_, stream->codecpar) < 0) {
        hlm_error("Failed to copy codec parameters to decoder context for stream index: {}", stream_index_);
        avcodec_free_context(&codec_context_);
        return false;
    }

    if (avcodec_open2(codec_context_, codec, nullptr) < 0) {
        hlm_error("Failed to open decoder for stream index: {}", stream_index_);
        avcodec_free_context(&codec_context_);
        return false;
    }

    return true;
}

bool HlmDecoder::initDecoder(AVFormatContext* format_context) {
    format_context_ = format_context;
    AVStream* stream = format_context->streams[stream_index_];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        hlm_error("Failed to find decoder for stream index: {}", stream_index_);
        return false;
    }

    codec_context_ = avcodec_alloc_context3(codec);
    if (!codec_context_) {
        hlm_error("Failed to allocate decoder context for stream index: {}", stream_index_);
        return false;
    }

    if (avcodec_parameters_to_context(codec_context_, stream->codecpar) < 0) {
        hlm_error("Failed to copy codec parameters to decoder context for stream index: {}", stream_index_);
        avcodec_free_context(&codec_context_);
        return false;
    }

    codec_context_->thread_count = 16;
    codec_context_->thread_type = FF_THREAD_FRAME;

    if (avcodec_open2(codec_context_, codec, nullptr) < 0) {
        hlm_error("Failed to open decoder for stream index: {}", stream_index_);
        avcodec_free_context(&codec_context_);
        return false;
    }

    return true;
}

bool HlmDecoder::decodePacket(AVPacket* pkt, AVFrame* frame) {
    if (avcodec_send_packet(codec_context_, pkt) < 0) {
        return false;
    }

    if (avcodec_receive_frame(codec_context_, frame) == 0) {
        return true;
    }

    return false;
}

AVCodecContext* HlmDecoder::getCodecContext() const {
    return codec_context_;
}

AVFormatContext* HlmDecoder::getFormatContext() const {
    return format_context_;
}


void HlmDecoder::flushDecoder(function<void(AVFrame*, int)> processFramesCallback) {
    AVFrame* frame = av_frame_alloc();
    int ret;

    if ((ret = avcodec_send_packet(codec_context_, nullptr)) < 0) {
        hlm_error("Error sending flush packet to decoder.");
        av_frame_free(&frame);
        return;
    }

    while (true) {
        ret = avcodec_receive_frame(codec_context_, frame);
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
            break;
        } else if (ret < 0) {
            hlm_error("Error receiving frame from decoder during flush.");
            break;
        }

        processFramesCallback(frame, stream_index_);
    }

    av_frame_free(&frame);
}

bool HlmDecoder::initScaler(int src_width, int src_height, AVPixelFormat src_pix_fmt,
                            int dst_width, int dst_height, AVPixelFormat dst_pix_fmt) {
    hlm_info("Initializing scaler from {}x{} ({}) to {}x{} ({}).",
             src_width, src_height, av_get_pix_fmt_name(src_pix_fmt), dst_width, dst_height, av_get_pix_fmt_name(dst_pix_fmt));
    sws_ctx_ = sws_getContext(src_width, src_height, src_pix_fmt,
                              dst_width, dst_height, dst_pix_fmt,
                              SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!sws_ctx_) {
        hlm_error("Failed to initialize the scaling context. ");
        return false;
    }

    scaled_frame_ = av_frame_alloc();
    if (!scaled_frame_) {
        hlm_error("Failed to allocate scaled frame.");
        return false;
    }

    scaled_frame_->width = dst_width;
    scaled_frame_->height = dst_height;
    scaled_frame_->format = dst_pix_fmt;
    if (av_frame_get_buffer(scaled_frame_, 32) < 0) {
        hlm_error("Failed to allocate buffer for scaled frame.");
        return false;
    }

    return true;
}

AVFrame* HlmDecoder::scaleFrame(AVFrame* frame) {
    if (!sws_ctx_) {
        hlm_error("Scaler context not initialized.");
        return nullptr;
    }

    sws_scale(sws_ctx_, frame->data, frame->linesize, 0, frame->height,
              scaled_frame_->data, scaled_frame_->linesize);

    scaled_frame_->pts = frame->pts;
    scaled_frame_->pkt_duration = frame->pkt_duration;
    return scaled_frame_;
}

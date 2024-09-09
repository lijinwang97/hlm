#include "hlm_decoder.h"

#include <iostream>

HlmDecoder::HlmDecoder(int stream_index)
    : codec_context_(nullptr), stream_index_(stream_index) {
}

HlmDecoder::~HlmDecoder() {
    if (codec_context_) {
        avcodec_free_context(&codec_context_);  // 手动释放 codec_context_
    }
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

    if (codec->type == AVMEDIA_TYPE_VIDEO) {
        media_type_ = "video";
    } else if (codec->type == AVMEDIA_TYPE_AUDIO) {
        media_type_ = "audio";
    }

    hlm_info("{} decoder initialized successfully for stream index: {}", media_type_, stream_index_);
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

AVCodecContext* HlmDecoder::getContext() const {
    return codec_context_;
}

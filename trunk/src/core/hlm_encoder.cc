#include "hlm_encoder.h"

#include "utils/hlm_config.h"
#include "utils/hlm_logger.h"

using namespace spdlog;

HlmEncoder::HlmEncoder()
    : ecodec_context_(nullptr), stream_(nullptr) {
}

HlmEncoder::~HlmEncoder() {
    if (ecodec_context_) {
        avcodec_free_context(&ecodec_context_);
    }

    if (sws_ctx_) {
        sws_freeContext(sws_ctx_);
    }
    if (scaled_frame_) {
        av_frame_free(&scaled_frame_);
        scaled_frame_ = nullptr; 
    }
}

bool HlmEncoder::initEncoder(AVFormatContext* format_context, AVCodecContext* decoder_context, const std::string& codec_name) {
    const AVCodec* codec = codec_name.empty() ? avcodec_find_encoder(decoder_context->codec_id) : avcodec_find_encoder_by_name(codec_name.c_str());
    if (!codec) {
        hlm_error("Failed to find encoder. codec_name:{}", codec_name);
        return false;
    }

    ecodec_context_ = avcodec_alloc_context3(codec);
    if (!ecodec_context_) {
        hlm_error("Failed to allocate encoder context.");
        return false;
    }

    if (codec_name == "png") {
        ecodec_context_->pix_fmt = AV_PIX_FMT_RGB24;
    } else {
        ecodec_context_->pix_fmt = decoder_context->pix_fmt;
    }

    if (codec->type == AVMEDIA_TYPE_VIDEO && codec_name != "png") {
        ecodec_context_->bit_rate = decoder_context->bit_rate;
    }

    ecodec_context_->width = decoder_context->width;
    ecodec_context_->height = decoder_context->height;

    if (codec->type == AVMEDIA_TYPE_AUDIO) {
        ecodec_context_->sample_fmt = decoder_context->sample_fmt;
        ecodec_context_->sample_rate = decoder_context->sample_rate;
        ecodec_context_->channel_layout = decoder_context->channel_layout;
        ecodec_context_->channels = decoder_context->channels;
    }

    AVRational framerate = decoder_context->framerate;
    if (framerate.num == 0 || framerate.den == 0) {
        framerate = AVRational{25, 1};
    }
    ecodec_context_->time_base = av_inv_q(framerate);
    ecodec_context_->framerate = framerate;

    if (codec->type == AVMEDIA_TYPE_VIDEO) {
        media_type_ = "video";
    } else if (codec->type == AVMEDIA_TYPE_AUDIO) {
        media_type_ = "audio";
    }

    ecodec_context_->thread_count = 16;
    ecodec_context_->thread_type = codec->type == AVMEDIA_TYPE_VIDEO ? FF_THREAD_FRAME : FF_THREAD_SLICE;

    if (avcodec_open2(ecodec_context_, codec, nullptr) < 0) {
        hlm_error("Failed to open encoder.");
        avcodec_free_context(&ecodec_context_);
        return false;
    }

    stream_ = avformat_new_stream(format_context, nullptr);
    if (!stream_) {
        hlm_error("Failed to create new stream.");
        avcodec_free_context(&ecodec_context_);
        return false;
    }

    stream_->time_base = ecodec_context_->time_base;
    if (avcodec_parameters_from_context(stream_->codecpar, ecodec_context_) < 0) {
        hlm_error("Failed to copy codec parameters to stream.");
        avcodec_free_context(&ecodec_context_);
        return false;
    }

    if (codec->type == AVMEDIA_TYPE_VIDEO) {
        hlm_info("video encoder init succ. stream info: \nIndex:{} \nCodec:{} \nWidth:{} \nHeight:{} \nBit rate:{} \nTime base:{}/{} \nFrame rate:{} \nGOP size:{} \nMax B frames:{} \nPixel format:{}\n",
                 stream_->index, avcodec_get_name(ecodec_context_->codec_id), ecodec_context_->width, ecodec_context_->height, ecodec_context_->bit_rate,
                 ecodec_context_->time_base.num, ecodec_context_->time_base.den, ecodec_context_->framerate.num, ecodec_context_->framerate.den, ecodec_context_->gop_size,
                 ecodec_context_->max_b_frames, av_get_pix_fmt_name(ecodec_context_->pix_fmt));
    } else if (codec->type == AVMEDIA_TYPE_AUDIO) {
        hlm_info("audio encoder init succ. stream info \nIndex:{} \nCodec:{} \nSample rate:{} \nChannels:{} \nChannel layout:{} \nBit rate:{} \nTime base:{}/{} \nSample format:{}\n",
                 stream_->index, avcodec_get_name(ecodec_context_->codec_id), ecodec_context_->sample_rate, ecodec_context_->channels, ecodec_context_->channel_layout,
                 ecodec_context_->bit_rate, ecodec_context_->time_base.num, ecodec_context_->time_base.den, av_get_sample_fmt_name(ecodec_context_->sample_fmt));
    }
    return true;
}

bool HlmEncoder::encodeFrame(AVFrame* frame, AVPacket* pkt) {
    if (avcodec_send_frame(ecodec_context_, frame) < 0) {
        return false;
    }

    // while (avcodec_receive_packet(ecodec_context_, pkt) == 0) {
    //     return true;
    // }

    int ret = avcodec_receive_packet(ecodec_context_, pkt);
    if (ret == 0) {
        return true;
    } else if (ret == AVERROR(EAGAIN)) {
        hlm_error("Encoder needs more input frames before it can output a packet.");
    } else if (ret == AVERROR_EOF) {
        hlm_info("Encoder reached the end of stream.");
    } else {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        hlm_error("Error receiving packet from encoder: {}", errbuf);
    }

    return false;
}

AVCodecContext* HlmEncoder::getContext() const {
    return ecodec_context_;
}

bool HlmEncoder::initScaler(int src_width, int src_height, AVPixelFormat src_pix_fmt,
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

AVFrame* HlmEncoder::scaleFrame(AVFrame* frame) {
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

#ifndef HLM_ENCODER_H
#define HLM_ENCODER_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include <memory>
#include <string>

class HlmEncoder {
   public:
    HlmEncoder();
    ~HlmEncoder();

    bool initEncoder(AVFormatContext* format_context, AVCodecContext* decoder_context, const std::string& codec_name = "");
    bool encodeFrame(AVFrame* frame, AVPacket* pkt);
    AVCodecContext* getContext() const;

    bool initScaler(int src_width, int src_height, AVPixelFormat src_pix_fmt,
                    int dst_width, int dst_height, AVPixelFormat dst_pix_fmt);
    AVFrame* scaleFrame(AVFrame* frame);

   private:
    AVCodecContext* ecodec_context_;
    AVStream* stream_;
    std::string media_type_;

    struct SwsContext* sws_ctx_;
    AVFrame* scaled_frame_;
};

#endif  // HLM_ENCODER_H

#ifndef HLM_ENCODER_H
#define HLM_ENCODER_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include <functional>
#include <memory>
#include <string>

using namespace std;

class HlmEncoder {
   public:
    HlmEncoder();
    ~HlmEncoder();

    bool initEncoderForImage(AVCodecContext* decoder_context, const std::string& codec_name = "");
    bool initEncoderForVideo(AVCodecContext* decoder_context, AVFormatContext* input_format_context,AVFormatContext* output_format_context, int stream_index);
    bool initEncoderForAudio(AVCodecContext* decoder_context, AVFormatContext* output_format_context);
    bool encodeFrame(AVFrame* frame, AVPacket* pkt);
    void flushEncoder(function<void(AVPacket*, int)> checkAndSavePacket);
    AVCodecContext* getContext() const;
    int getStreamIndex() const { return stream_index_; }

    bool initScaler(int src_width, int src_height, AVPixelFormat src_pix_fmt,
                    int dst_width, int dst_height, AVPixelFormat dst_pix_fmt);
    AVFrame* scaleFrame(AVFrame* frame);

   private:
    AVCodecContext* ecodec_context_;
    AVStream* stream_;

    struct SwsContext* sws_ctx_;
    AVFrame* scaled_frame_;
    int stream_index_ = -1;
};

#endif  // HLM_ENCODER_H

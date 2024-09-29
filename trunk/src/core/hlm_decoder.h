#ifndef HLM_DECODER_H
#define HLM_DECODER_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include <functional>
#include <memory>

#include "utils/hlm_logger.h"

using namespace std;

class HlmDecoder {
   public:
    HlmDecoder(int stream_index);
    HlmDecoder(AVMediaType media_type);
    ~HlmDecoder();

    bool openInputStream(const string& input_url);
    bool initDecoder();
    bool initDecoder(AVFormatContext* format_context);
    bool decodePacket(AVPacket* pkt, AVFrame* frame);
    AVCodecContext* getCodecContext() const;
    AVFormatContext* getFormatContext() const;
    void flushDecoder(function<void(AVFrame*, int)> processFramesCallback);

    bool initScaler(int src_width, int src_height, AVPixelFormat src_pix_fmt,
                    int dst_width, int dst_height, AVPixelFormat dst_pix_fmt);
    AVFrame* scaleFrame(AVFrame* frame);

   private:
    AVCodecContext* codec_context_;
    AVFormatContext* format_context_;
    AVMediaType media_type_;
    int stream_index_ = -1;

    struct SwsContext* sws_ctx_;
    AVFrame* scaled_frame_;
};

#endif  // HLM_DECODER_H

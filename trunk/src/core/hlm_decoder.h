#ifndef HLM_DECODER_H
#define HLM_DECODER_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
}

#include <memory>
#include <functional>

#include "utils/hlm_logger.h"

using namespace std;

class HlmDecoder {
   public:
    HlmDecoder(int stream_index);
    ~HlmDecoder();

    bool initDecoder(AVFormatContext* format_context);
    bool decodePacket(AVPacket* pkt, AVFrame* frame);
    AVCodecContext* getContext() const;
    void flushDecoder(function<void(AVFrame*, int)> processFramesCallback);

   private:
    AVCodecContext* codec_context_;
    AVFormatContext* format_context_;
    string media_type_;
    int stream_index_ = -1;
};

#endif  // HLM_DECODER_H

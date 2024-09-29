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

struct EncoderParams {
    int width = 0;                               // 视频宽度
    int height = 0;                              // 视频高度
    int fps = 30;                                // 帧率
    int video_bitrate = 2000000;                 // 视频码率
    AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P;  // 像素格式
    string video_encoder_name = "libx264";       // 视频编码器名，h264软编：libx264，h264硬编：h264_nvenc，h265软编：libx265，h265硬编：hevc_nvenc

    int sample_rate = 44100;                         // 音频采样率
    int channels = 2;                                // 音频通道数
    int audio_bitrate = 128000;                      // 音频码率
    AVSampleFormat sample_fmt = AV_SAMPLE_FMT_FLTP;  // 音频格式
    string audio_encoder_name = "aac";               // 音频编码器名，默认AAC
};

class HlmEncoder {
   public:
    HlmEncoder();
    ~HlmEncoder();

    bool initEncoderForImage(AVCodecContext* decoder_context, const string& codec_name = "");
    bool initVideoEncoder(const EncoderParams& params, AVFormatContext* output_format_context);
    bool initAudioEncoder(const EncoderParams& params, AVFormatContext* output_format_context);
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

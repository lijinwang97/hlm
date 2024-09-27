#include "hlm_recording_executor.h"

#include <filesystem>

#include "utils/hlm_logger.h"
#include "utils/hlm_time.h"

HlmRecordingExecutor::HlmRecordingExecutor(const string& stream_url, const string& output_dir, const string& filename, const string& recording_method)
    : HlmExecutor(stream_url, output_dir, filename, recording_method), recording_method_(recording_method) {}

bool HlmRecordingExecutor::init() {
    if (!ensureDirectoryExists(output_dir_)) {
        return false;
    }

    if (!openInputStream()) {
        hlm_error("Failed to open input stream.");
        return false;
    }

    if (!findStreams()) {
        hlm_error("Failed to find video/audio stream.");
        return false;
    }

    if (!initOutputFile()) {
        hlm_error("Failed to initialize output file.");
        return false;
    }

    running_ = true;
    return true;
}

bool HlmRecordingExecutor::initOutputFile() {
    if (avformat_alloc_output_context2(&output_format_context_, nullptr, nullptr, filename_.c_str()) < 0) {
        hlm_error("Failed to allocate output format context.");
        return false;
    }

    AVStream* output_video_stream = avformat_new_stream(output_format_context_, nullptr);
    avcodec_parameters_copy(output_video_stream->codecpar, input_format_context_->streams[input_video_stream_index_]->codecpar);
    AVRational input_frame_rate = av_guess_frame_rate(input_format_context_, input_format_context_->streams[input_video_stream_index_], nullptr);
    output_video_stream->r_frame_rate = input_frame_rate;
    output_video_stream->avg_frame_rate = input_frame_rate;
    output_video_stream->time_base = av_inv_q(input_frame_rate);
    output_video_stream_index_ = output_video_stream->index;

    if (recording_method_ == HlmRecordingMethod::Hls) {
        av_opt_set(output_format_context_->priv_data, "hls_time", "5", 0);
        av_opt_set(output_format_context_->priv_data, "hls_list_size", "0", 0);
        setHlsSegmentFilename();
    }

    hlm_info("Input video stream time_base: {}/{} | Output video stream time_base: {}/{}",
             input_format_context_->streams[input_video_stream_index_]->time_base.num, input_format_context_->streams[input_video_stream_index_]->time_base.den,
             output_video_stream->time_base.num, output_video_stream->time_base.den);
    hlm_info("Input video codec parameters: codec_type={}, bit_rate={}, width={}, height={}",
             (int)input_format_context_->streams[input_video_stream_index_]->codecpar->codec_type, input_format_context_->streams[input_video_stream_index_]->codecpar->bit_rate,
             input_format_context_->streams[input_video_stream_index_]->codecpar->width, input_format_context_->streams[input_video_stream_index_]->codecpar->height);
    hlm_info("Output video codec parameters: codec_type={}, bit_rate={}, width={}, height={}",
             (int)output_video_stream->codecpar->codec_type, output_video_stream->codecpar->bit_rate,
             output_video_stream->codecpar->width, output_video_stream->codecpar->height);

    AVStream* output_audio_stream = avformat_new_stream(output_format_context_, nullptr);
    avcodec_parameters_copy(output_audio_stream->codecpar, input_format_context_->streams[input_audio_stream_index_]->codecpar);
    output_audio_stream->time_base = input_format_context_->streams[input_audio_stream_index_]->time_base;
    output_audio_stream_index_ = output_audio_stream->index;
    hlm_info("Input audio stream time_base: {}/{} | Output audio stream time_base: {}/{}",
             input_format_context_->streams[input_audio_stream_index_]->time_base.num, input_format_context_->streams[input_audio_stream_index_]->time_base.den,
             output_audio_stream->time_base.num, output_audio_stream->time_base.den);
    hlm_info("Input audio codec parameters: codec_type={}, bit_rate={}, sample_rate={}, channels={}",
             (int)input_format_context_->streams[input_audio_stream_index_]->codecpar->codec_type, input_format_context_->streams[input_audio_stream_index_]->codecpar->bit_rate,
             input_format_context_->streams[input_audio_stream_index_]->codecpar->sample_rate, input_format_context_->streams[input_audio_stream_index_]->codecpar->channels);
    hlm_info("Output audio codec parameters: codec_type={}, bit_rate={}, sample_rate={}, channels={}",
             (int)output_audio_stream->codecpar->codec_type, output_audio_stream->codecpar->bit_rate,
             output_audio_stream->codecpar->sample_rate, output_audio_stream->codecpar->channels);

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

void HlmRecordingExecutor::execute() {
    hlm_info("Starting {} for stream: {}", media_method_, stream_url_);
    if (!init()) {
        hlm_error("{} initialization failed for stream: {}", media_method_, stream_url_);
        return;
    }

    AVPacket* packet = av_packet_alloc();
    while (isRunning() && av_read_frame(input_format_context_, packet) >= 0) {
        updateStartTime();
        int64_t pts = packet->pts;
        double frame_time = pts * av_q2d(input_format_context_->streams[packet->stream_index]->time_base);
        hlm_debug("Processing {} recording. PTS: {}, time: {}s, stream index:{}", media_method_, pts, frame_time, packet->stream_index);

        if (packet->stream_index == input_video_stream_index_) {
            packet->stream_index = output_video_stream_index_;
            checkAndSavePacket(packet, output_video_stream_index_);
        }

        if (packet->stream_index == input_audio_stream_index_) {
            packet->stream_index = output_audio_stream_index_;
            checkAndSavePacket(packet, output_audio_stream_index_);
        }
        av_packet_unref(packet);
    }
    av_packet_free(&packet);
    endRecording();
}

void HlmRecordingExecutor::setHlsSegmentFilename() {
    size_t pos = filename_.find_last_of('.');
    if (pos != string::npos) {
        filename_ = filename_.substr(0, pos);
    }
    filename_ += "_%03d.ts";
    av_opt_set(output_format_context_->priv_data, "hls_segment_filename", filename_.c_str(), 0);
}

void HlmRecordingExecutor::endRecording() {
    if (av_write_trailer(output_format_context_) < 0) {
        hlm_error("Failed to write trailer to output file.");
    }

    if (!(output_format_context_->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&output_format_context_->pb);
    }
    hlm_info("Recording {} end and output file closed for stream: {}", media_method_, stream_url_);
}

HlmMp4RecordingExecutor::HlmMp4RecordingExecutor(const string& stream_url, const string& output_dir, const string& filename, const string& recording_method)
    : HlmRecordingExecutor(stream_url, output_dir, filename, recording_method) {
}

void HlmMp4RecordingExecutor::checkAndSavePacket(AVPacket* encoded_packet, int stream_index) {
    int64_t old_pts_time = encoded_packet->pts;
    int64_t old_dts_time = encoded_packet->dts;

    if (stream_index == output_video_stream_index_) {
        av_packet_rescale_ts(encoded_packet, input_format_context_->streams[input_video_stream_index_]->time_base, output_format_context_->streams[stream_index]->time_base);
    } else if (stream_index == output_audio_stream_index_) {
        av_packet_rescale_ts(encoded_packet, input_format_context_->streams[input_video_stream_index_]->time_base, output_format_context_->streams[stream_index]->time_base);
    }

    AVRational output_time_base = output_format_context_->streams[stream_index]->time_base;
    double new_pts_time = encoded_packet->pts * av_q2d(output_time_base);
    double new_dts_time = encoded_packet->dts * av_q2d(output_time_base);

    hlm_debug("Rescaled Packet info: Stream Index: {}, old PTS: {}, old DTS:{}, PTS: {} ({}s), DTS: {} ({}s), Time Base: {}/{}",
              stream_index, old_pts_time, old_dts_time, encoded_packet->pts, new_pts_time, encoded_packet->dts, new_dts_time,
              output_time_base.num, output_time_base.den);

    if (av_interleaved_write_frame(output_format_context_, encoded_packet) < 0) {
        hlm_error("Failed to write packet to output file for stream index: {}", stream_index);
    }
}

HlmHlsRecordingExecutor::HlmHlsRecordingExecutor(const string& stream_url, const string& output_dir, const string& filename, const string& recording_method)
    : HlmRecordingExecutor(stream_url, output_dir, filename, recording_method) {
}

void HlmHlsRecordingExecutor::checkAndSavePacket(AVPacket* encoded_packet, int stream_index) {
    int64_t old_pts_time = encoded_packet->pts;
    int64_t old_dts_time = encoded_packet->dts;

    if (stream_index == output_video_stream_index_) {
        av_packet_rescale_ts(encoded_packet, input_format_context_->streams[input_video_stream_index_]->time_base, output_format_context_->streams[stream_index]->time_base);
    } else if (stream_index == output_audio_stream_index_) {
        av_packet_rescale_ts(encoded_packet, input_format_context_->streams[input_video_stream_index_]->time_base, output_format_context_->streams[stream_index]->time_base);
    }

    AVRational output_time_base = output_format_context_->streams[stream_index]->time_base;
    double new_pts_time = encoded_packet->pts * av_q2d(output_time_base);
    double new_dts_time = encoded_packet->dts * av_q2d(output_time_base);

    hlm_debug("Rescaled Packet info: Stream Index: {}, old PTS: {}, old DTS:{}, PTS: {} ({}s), DTS: {} ({}s), Time Base: {}/{}",
              stream_index, old_pts_time, old_dts_time, encoded_packet->pts, new_pts_time, encoded_packet->dts, new_dts_time,
              output_time_base.num, output_time_base.den);

    if (av_interleaved_write_frame(output_format_context_, encoded_packet) < 0) {
        hlm_error("Failed to write packet to output file for stream index: {}", stream_index);
    }
}

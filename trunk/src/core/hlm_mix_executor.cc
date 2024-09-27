#include "hlm_mix_executor.h"

#include "utils/hlm_logger.h"

HlmMixExecutor::HlmMixExecutor(const HlmMixTaskParams& params)
    : HlmExecutor(), params_(params) {
}

bool HlmMixExecutor::init() {
    hlm_info("Initializing mixing task for output URL: {}", params_.output_url);
    // 初始化解码器、编码器等混流相关的组件
    // 如果初始化失败，返回 false
    return true;
}

bool HlmMixExecutor::initOutputFile() {
    hlm_info("Initializing output file for mixing task.");
    // 设置输出流、编码器等
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
        hlm_info("Started processing stream: {}, URL: {}, width:{}, height:{}, x:{}, y:{}, z_index:{}", stream.id, stream.url,stream.width,stream.height,stream.x,stream.y,stream.z_index );
    }

    hlm_info("Mixing task completed with output URL: {}", params_.output_url);
}

void HlmMixExecutor::updateStreams(const HlmMixTaskParams& params){

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

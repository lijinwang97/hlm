#include "hlm_screenshot_executor.h"
#include "utils/hlm_logger.h"

// 通用 ScreenshotExecutor 基类的实现
ScreenshotExecutor::ScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix)
    : stream_url_(stream_url), output_dir_(output_dir), filename_prefix_(filename_prefix) {}

void ScreenshotExecutor::initScreenshotEnvironment() {
    hlm_info("Initializing screenshot environment for stream: {}", stream_url_);
    // 这里可以做一些初始化工作，比如检查 stream_url 是否有效，准备输出目录等
}

void ScreenshotExecutor::saveFrameAsImage(int frame_number) {
    string output_file = output_dir_ + "/" + filename_prefix_ + "_" + std::to_string(frame_number) + ".png";
    hlm_info("Saving frame as image: {}", output_file);
    // 实际的保存逻辑
}

// 按时间间隔截图的实现
HlmIntervalScreenshotExecutor::HlmIntervalScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int interval)
    : ScreenshotExecutor(stream_url, output_dir, filename_prefix), interval_(interval) {}

void HlmIntervalScreenshotExecutor::execute() {
    initScreenshotEnvironment();
    int frame_count = 0;
    while (running_) {
        hlm_info("Capturing frame from stream: {}", stream_url_);
        saveFrameAsImage(frame_count++);
        std::this_thread::sleep_for(std::chrono::seconds(interval_));
    }
}

void HlmIntervalScreenshotExecutor::stop() {
    running_ = false;
}

// 按百分比截图的实现
HlmPercentageScreenshotExecutor::HlmPercentageScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int percentage)
    : ScreenshotExecutor(stream_url, output_dir, filename_prefix), percentage_(percentage) {}

void HlmPercentageScreenshotExecutor::execute() {
    initScreenshotEnvironment();
    hlm_info("Capturing frame for {}% of the stream.", percentage_);
    saveFrameAsImage(0);  // 示例
}

void HlmPercentageScreenshotExecutor::stop() {
    hlm_info("Percentage screenshot task doesn't need explicit stop.");
}

// 立即截图的实现
HlmImmediateScreenshotExecutor::HlmImmediateScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix)
    : ScreenshotExecutor(stream_url, output_dir, filename_prefix) {}

void HlmImmediateScreenshotExecutor::execute() {
    initScreenshotEnvironment();
    hlm_info("Capturing immediate screenshot from stream: {}", stream_url_);
    saveFrameAsImage(0);
}

void HlmImmediateScreenshotExecutor::stop() {
    // 立即截图没有执行中的停止需求
    hlm_info("Immediate screenshot task doesn't need explicit stop.");
}

// 指定时间点截图的实现
HlmSpecificTimeScreenshotExecutor::HlmSpecificTimeScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int time_second)
    : ScreenshotExecutor(stream_url, output_dir, filename_prefix), time_second_(time_second) {}

void HlmSpecificTimeScreenshotExecutor::execute() {
    initScreenshotEnvironment();
    hlm_info("Capturing frame at {} seconds from stream: {}", time_second_, stream_url_);
    saveFrameAsImage(0);
}

void HlmSpecificTimeScreenshotExecutor::stop() {
    // 没有执行中的停止需求
    hlm_info("Specific time screenshot task doesn't need explicit stop.");
}

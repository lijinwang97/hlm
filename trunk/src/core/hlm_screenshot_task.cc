#include "hlm_screenshot_task.h"

#include "utils/hlm_logger.h"

HlmScreenshotTask::HlmScreenshotTask(const string& stream_url, const string& method)
    : HlmTask(TaskType::Screenshot, stream_url, method) {}

void HlmScreenshotTask::execute() {
    if (!executor_) {
        executor_ = createExecutor();
    }
    executor_->execute();
}

void HlmScreenshotTask::stop() {
    if (executor_ && executor_->isRunning()) {
        executor_->stop();
    }
}

HlmIntervalScreenshotTask::HlmIntervalScreenshotTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix, int interval)
    : HlmScreenshotTask(stream_url, method), output_dir_(output_dir), filename_prefix_(filename_prefix), interval_(interval) {
}

unique_ptr<HlmScreenshotExecutor> HlmIntervalScreenshotTask::createExecutor() {
    return make_unique<HlmIntervalScreenshotExecutor>(getStreamUrl(), output_dir_, filename_prefix_, interval_, HlmScreenshotMethod::Interval);
}

HlmPercentageScreenshotTask::HlmPercentageScreenshotTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix, int percentage)
    : HlmScreenshotTask(stream_url, method), output_dir_(output_dir), filename_prefix_(filename_prefix), percentage_(percentage) {
}

unique_ptr<HlmScreenshotExecutor> HlmPercentageScreenshotTask::createExecutor() {
    return make_unique<HlmPercentageScreenshotExecutor>(getStreamUrl(), output_dir_, filename_prefix_, percentage_, HlmScreenshotMethod::Percentage);
}

HlmImmediateScreenshotTask::HlmImmediateScreenshotTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix)
    : HlmScreenshotTask(stream_url, method), output_dir_(output_dir), filename_prefix_(filename_prefix) {
}

unique_ptr<HlmScreenshotExecutor> HlmImmediateScreenshotTask::createExecutor() {
    return make_unique<HlmImmediateScreenshotExecutor>(getStreamUrl(), output_dir_, filename_prefix_, HlmScreenshotMethod::Immediate);
}

HlmSpecificTimeScreenshotTask::HlmSpecificTimeScreenshotTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix, int time_second)
    : HlmScreenshotTask(stream_url, method), output_dir_(output_dir), filename_prefix_(filename_prefix), time_second_(time_second) {
}

unique_ptr<HlmScreenshotExecutor> HlmSpecificTimeScreenshotTask::createExecutor() {
    return make_unique<HlmSpecificTimeScreenshotExecutor>(getStreamUrl(), output_dir_, filename_prefix_, time_second_, HlmScreenshotMethod::SpecificTime);
}
#include "hlm_screenshot_strategy.h"
#include "hlm_screenshot_task.h"

shared_ptr<HlmTask> HlmIntervalScreenshotStrategy::createTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix, const json::rvalue& body) {
    int interval = body["interval"].i();
    if (interval <= 0) {
        throw invalid_argument("Interval must be positive.");
    }
    return make_shared<HlmIntervalScreenshotTask>(stream_url, method, output_dir, filename_prefix, interval);
}

shared_ptr<HlmTask> HlmPercentageScreenshotStrategy::createTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix, const json::rvalue& body) {
    int percentage = body["percentage"].i();
    if (percentage <= 0 || percentage > 100) {
        throw invalid_argument("Percentage must be between 1 and 100.");
    }
    return make_shared<HlmPercentageScreenshotTask>(stream_url, method, output_dir, filename_prefix, percentage);
}

shared_ptr<HlmTask> HlmImmediateScreenshotStrategy::createTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix, const json::rvalue& body) {
    return make_shared<HlmImmediateScreenshotTask>(stream_url, method, output_dir, filename_prefix);
}

shared_ptr<HlmTask> HlmSpecificTimeScreenshotStrategy::createTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix, const json::rvalue& body) {
    int timeSecond = body["time_second"].i();
    if (timeSecond < 0) {
        throw invalid_argument("time_second must be non-negative.");
    }
    return make_shared<HlmSpecificTimeScreenshotTask>(stream_url, method, output_dir, filename_prefix, timeSecond);
}

shared_ptr<HlmScreenshotStrategy> HlmScreenshotStrategyFactory::createStrategy(const string& method) {
    if (method == HlmScreenshotMethod::Interval) {
        return make_shared<HlmIntervalScreenshotStrategy>();
    } else if (method == HlmScreenshotMethod::Percentage) {
        return make_shared<HlmPercentageScreenshotStrategy>();
    } else if (method == HlmScreenshotMethod::Immediate) {
        return make_shared<HlmImmediateScreenshotStrategy>();
    } else if (method == HlmScreenshotMethod::SpecificTime) {
        return make_shared<HlmSpecificTimeScreenshotStrategy>();
    } else {
        throw invalid_argument("Invalid screenshot method.");
    }
}
#include "hlm_screenshot_strategy.h"

#include "hlm_task.h"

shared_ptr<HlmTask> HlmIntervalScreenshotStrategy::createTask(const string& streamUrl, const string& outputDir, const string& filenamePrefix, const json::rvalue& body) {
    int interval = body["interval"].i();
    if (interval <= 0) {
        throw invalid_argument("Interval must be positive.");
    }
    return make_shared<HlmScreenshotTask>(streamUrl, outputDir, filenamePrefix, interval);
}

shared_ptr<HlmTask> HlmPercentageScreenshotStrategy::createTask(const string& streamUrl, const string& outputDir, const string& filenamePrefix, const json::rvalue& body) {
    int percentage = body["percentage"].i();
    if (percentage <= 0 || percentage > 100) {
        throw invalid_argument("Percentage must be between 1 and 100.");
    }
    return make_shared<HlmPercentageScreenshotTask>(streamUrl, outputDir, filenamePrefix, percentage);
}

shared_ptr<HlmTask> HlmImmediateScreenshotStrategy::createTask(const string& streamUrl, const string& outputDir, const string& filenamePrefix, const json::rvalue& body) {
    return make_shared<HlmImmediateScreenshotTask>(streamUrl, outputDir, filenamePrefix);
}

shared_ptr<HlmTask> HlmSpecificTimeScreenshotStrategy::createTask(const string& streamUrl, const string& outputDir, const string& filenamePrefix, const json::rvalue& body) {
    int timeSecond = body["time_second"].i();
    if (timeSecond < 0) {
        throw invalid_argument("time_second must be non-negative.");
    }
    return make_shared<HlmSpecificTimeScreenshotTask>(streamUrl, outputDir, filenamePrefix, timeSecond);
}

shared_ptr<HlmScreenshotStrategy> HlmScreenshotStrategyFactory::createStrategy(const string& method) {
    if (method == "interval") {
        return make_shared<HlmIntervalScreenshotStrategy>();
    } else if (method == "percentage") {
        return make_shared<HlmPercentageScreenshotStrategy>();
    } else if (method == "immediate") {
        return make_shared<HlmImmediateScreenshotStrategy>();
    } else if (method == "specific_time") {
        return make_shared<HlmSpecificTimeScreenshotStrategy>();
    } else {
        throw invalid_argument("Invalid screenshot method.");
    }
}
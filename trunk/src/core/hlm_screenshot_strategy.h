#ifndef HLM_SCREENSHOT_STRATEGY_H
#define HLM_SCREENSHOT_STRATEGY_H

#include <memory>
#include <string>

#include "crow.h"
#include "hlm_task.h"

using namespace crow;
using namespace std;

namespace HlmScreenshotMethod {
    const string Interval = "interval";
    const string Percentage = "percentage";
    const string Immediate = "immediate";
    const string SpecificTime = "specific_time";
}

// 策略基类 HlmScreenshotStrategy
class HlmScreenshotStrategy {
   public:
    virtual ~HlmScreenshotStrategy() = default;
    virtual shared_ptr<HlmTask> createTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix, const json::rvalue& body) = 0;
};

// 按时间间隔截图策略
class HlmIntervalScreenshotStrategy : public HlmScreenshotStrategy {
   public:
    shared_ptr<HlmTask> createTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix, const json::rvalue& body) override;
};

// 按百分比截图策略
class HlmPercentageScreenshotStrategy : public HlmScreenshotStrategy {
   public:
    shared_ptr<HlmTask> createTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix, const json::rvalue& body) override;
};

// 立即截图策略
class HlmImmediateScreenshotStrategy : public HlmScreenshotStrategy {
   public:
    shared_ptr<HlmTask> createTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix, const json::rvalue& body) override;
};

// 指定时间点截图策略
class HlmSpecificTimeScreenshotStrategy : public HlmScreenshotStrategy {
   public:
    shared_ptr<HlmTask> createTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix, const json::rvalue& body) override;
};

// 使用工厂创建策略
class HlmScreenshotStrategyFactory {
   public:
    static shared_ptr<HlmScreenshotStrategy> createStrategy(const string& method);
};

#endif  // HLM_SCREENSHOT_STRATEGY_H

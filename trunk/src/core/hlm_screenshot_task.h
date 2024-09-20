#ifndef HLM_SCREENSHOT_TASK_H
#define HLM_SCREENSHOT_TASK_H

#include "hlm_task.h"
#include "hlm_screenshot_executor.h"
#include <memory>
#include <string>

// 处理截图任务的基类 HlmScreenshotTask
class HlmScreenshotTask : public HlmTask {
   public:
    HlmScreenshotTask(const string& stream_url, const string& method);

    void execute() override;
    void stop() override;

   protected:
    virtual unique_ptr<HlmScreenshotExecutor> createExecutor() = 0;

    unique_ptr<HlmScreenshotExecutor> executor_;
};

// 按时间间隔截图
class HlmIntervalScreenshotTask : public HlmScreenshotTask {
   public:
    HlmIntervalScreenshotTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix, int interval);

   protected:
    unique_ptr<HlmScreenshotExecutor> createExecutor() override;

   private:
    string output_dir_;
    string filename_prefix_;
    int interval_;
};

// 按百分比截图
class HlmPercentageScreenshotTask : public HlmScreenshotTask {
   public:
    HlmPercentageScreenshotTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix, int percentage);

   protected:
    unique_ptr<HlmScreenshotExecutor> createExecutor() override;

   private:
    string output_dir_;
    string filename_prefix_;
    int percentage_;
};

// 立即截图
class HlmImmediateScreenshotTask : public HlmScreenshotTask {
   public:
    HlmImmediateScreenshotTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix);

   protected:
    unique_ptr<HlmScreenshotExecutor> createExecutor() override;

   private:
    string output_dir_;
    string filename_prefix_;
};

// 指定时间点截图
class HlmSpecificTimeScreenshotTask : public HlmScreenshotTask {
   public:
    HlmSpecificTimeScreenshotTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix, int time_second);

   protected:
    unique_ptr<HlmScreenshotExecutor> createExecutor() override;

   private:
    string output_dir_;
    string filename_prefix_;
    int time_second_;
};

#endif  // HLM_SCREENSHOT_TASK_H
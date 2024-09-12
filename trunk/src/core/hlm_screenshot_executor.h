#ifndef HLM_SCREENSHOT_EXECUTOR_H
#define HLM_SCREENSHOT_EXECUTOR_H

#include <string>
#include <chrono>
#include <thread>

using namespace std;

// 通用的 ScreenshotExecutor 基类
class ScreenshotExecutor {
public:
    ScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix);
    virtual ~ScreenshotExecutor() = default;

    virtual void execute() = 0;
    virtual void stop() = 0;

protected:
    string stream_url_;
    string output_dir_;
    string filename_prefix_;

    void initScreenshotEnvironment();
    void saveFrameAsImage(int frame_number);
};

// 按时间间隔截图
class HlmIntervalScreenshotExecutor : public ScreenshotExecutor {
public:
    HlmIntervalScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int interval);
    void execute() override;
    void stop() override;

private:
    int interval_;
    bool running_ = true;
};

// 按百分比截图
class HlmPercentageScreenshotExecutor : public ScreenshotExecutor {
public:
    HlmPercentageScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int percentage);
    void execute() override;
    void stop() override;

private:
    int percentage_;
};

// 立即截图
class HlmImmediateScreenshotExecutor : public ScreenshotExecutor {
public:
    HlmImmediateScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix);
    void execute() override;
    void stop() override;
};

// 指定时间点截图
class HlmSpecificTimeScreenshotExecutor : public ScreenshotExecutor {
public:
    HlmSpecificTimeScreenshotExecutor(const string& stream_url, const string& output_dir, const string& filename_prefix, int time_second);
    void execute() override;
    void stop() override;

private:
    int time_second_;
};

#endif // HLM_SCREENSHOT_EXECUTOR_H

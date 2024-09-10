#ifndef TASK_H
#define TASK_H

#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

using namespace std;

// 基类 HlmTask
class HlmTask {
   public:
    enum class TaskType { Screenshot,
                          Recording,
                          Mixing };

    HlmTask(TaskType type);
    virtual ~HlmTask() = default;

    TaskType getType() const;

    // 纯虚函数，让每个子类实现自己的参数处理
    virtual void execute() = 0;

   protected:
    TaskType type_;
};

// HlmScreenshotTask（按时间间隔截图）
class HlmScreenshotTask : public HlmTask {
public:
    HlmScreenshotTask(const  string& stream_url, const  string& output_dir, const  string& filename_prefix, int interval);

    void execute() override;

private:
     string stream_url_;
     string output_dir_;
     string filename_prefix_;
    int interval_;
};

// HlmPercentageScreenshotTask（按百分比截图）
class HlmPercentageScreenshotTask : public HlmTask {
public:
    HlmPercentageScreenshotTask(const  string& stream_url, const  string& output_dir, const  string& filename_prefix, int percentage);

    void execute() override;

private:
     string stream_url_;
     string output_dir_;
     string filename_prefix_;
    int percentage_;
};

// HlmImmediateScreenshotTask（立即截图）
class HlmImmediateScreenshotTask : public HlmTask {
public:
    HlmImmediateScreenshotTask(const  string& stream_url, const  string& output_dir, const  string& filename_prefix);

    void execute() override;

private:
     string stream_url_;
     string output_dir_;
     string filename_prefix_;
};

// HlmSpecificTimeScreenshotTask（指定时间点截图）
class HlmSpecificTimeScreenshotTask : public HlmTask {
public:
    HlmSpecificTimeScreenshotTask(const  string& stream_url, const  string& output_dir, const  string& filename_prefix, int timeSecond);

    void execute() override;

private:
     string stream_url_;
     string output_dir_;
     string filename_prefix_;
    int timeSecond_;
};


// 录像任务
class HlmRecordingTask : public HlmTask {
   public:
    HlmRecordingTask(const string& stream_url, const string& outputPath, int duration);

    void execute() override;

   private:
    string stream_url_;
    string outputPath_;
    int duration_;
};

// 混流任务
class HlmMixingTask : public HlmTask {
   public:
    HlmMixingTask(const string& input1, const string& input2, const string& output);

    void execute() override;

   private:
    string input1_;
    string input2_;
    string output_;
};

// HlmTaskManager 管理任务队列
class HlmTaskManager {
   public:
    HlmTaskManager(int max_tasks);

    bool addTask(shared_ptr<HlmTask> task);
    void taskCompleted(shared_ptr<HlmTask> task);

   private:
    void executeTask(shared_ptr<HlmTask> task);

    queue<shared_ptr<HlmTask>> task_queue_;
    vector<shared_ptr<HlmTask>> active_tasks_;
    int max_tasks_;
    mutex mutex_;
};

#endif  // TASK_H

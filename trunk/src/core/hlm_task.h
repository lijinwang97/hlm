#ifndef HLM_TASK_H
#define HLM_TASK_H

#include <algorithm>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std;

// 基类 HlmTask
class HlmTask {
   public:
    enum class TaskType { Screenshot,
                          Recording,
                          Mixing };

    HlmTask(TaskType type , const std::string& stream_url, const std::string& method);
    virtual ~HlmTask() = default;

    TaskType getType() const;
    const std::string& getStreamUrl() const;
    const std::string& getMethod() const;

    virtual void execute() = 0;

   private:
    TaskType type_;
    std::string stream_url_;
    std::string method_;
};

// 按时间间隔截图
class HlmScreenshotTask : public HlmTask {
   public:
    HlmScreenshotTask(const string& stream_url, const string& method, const string& output_dir, const string& filename_prefix, int interval);

    void execute() override;

   private:
    string output_dir_;
    string filename_prefix_;
    int interval_;
};

// 按百分比截图
class HlmPercentageScreenshotTask : public HlmTask {
   public:
    HlmPercentageScreenshotTask(const string& stream_url,const string& method, const string& output_dir, const string& filename_prefix, int percentage);

    void execute() override;

   private:
    string output_dir_;
    string filename_prefix_;
    int percentage_;
};

// 立即截图
class HlmImmediateScreenshotTask : public HlmTask {
   public:
    HlmImmediateScreenshotTask(const string& stream_url, const string& method,const string& output_dir, const string& filename_prefix);

    void execute() override;

   private:
    string output_dir_;
    string filename_prefix_;
};

// 指定时间点截图
class HlmSpecificTimeScreenshotTask : public HlmTask {
   public:
    HlmSpecificTimeScreenshotTask(const string& stream_url,const string& method, const string& output_dir, const string& filename_prefix, int timeSecond);

    void execute() override;

   private:
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

namespace HlmTaskAction {
    const string Start = "start";
    const string Stop = "stop";
}

// HlmTaskManager 管理任务队列
class HlmTaskManager {
   public:
    HlmTaskManager(int max_tasks);

    bool addTask(shared_ptr<HlmTask> task, const string& streamUrl, const string& method);
    bool removeTask(const string& streamUrl, const string& method);
    void taskCompleted(shared_ptr<HlmTask> task, const string& task_key);

   private:
    void executeTask(shared_ptr<HlmTask> task, const string& task_key);
    string createTaskKey(const string& streamUrl, const string& method);

    queue<shared_ptr<HlmTask>> task_queue_;
    vector<shared_ptr<HlmTask>> active_tasks_;
    unordered_map<string, bool> active_task_keys_;

    int max_tasks_;
    mutex mutex_;
};

#endif  // HLM_TASK_H

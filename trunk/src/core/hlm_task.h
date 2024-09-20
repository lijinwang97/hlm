#ifndef HLM_TASK_H
#define HLM_TASK_H

#include <algorithm>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
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

    HlmTask(TaskType type, const std::string& stream_url, const string& method);
    virtual ~HlmTask() = default;

    TaskType getType() const;
    const std::string& getStreamUrl() const;
    const std::string& getMethod() const;
    void setCancelled(bool cancelled);
    bool isCancelled() const;

    virtual void execute() = 0;
    virtual void stop() = 0;

   private:
    TaskType type_;
    std::string stream_url_;
    std::string method_;
    bool cancelled_;
};




// 混流任务
class HlmMixingTask : public HlmTask {
   public:
    HlmMixingTask(const string& input1, const string& input2, const string& output);

    void execute() override;

   private:
    string input1_;
    string input2_;
};

namespace HlmTaskAction {
const string Start = "start";
const string Stop = "stop";
}  // namespace HlmTaskAction

enum class HlmTaskAddStatus {
    TaskAlreadyRunning,  // 已有相同任务在执行
    TaskQueued,          // 任务已加入队列
    TaskStarted,         // 任务已启动
    QueueFull            // 队列已满，无法加入
};

// HlmTaskManager 管理任务队列
class HlmTaskManager {
   public:
    HlmTaskManager(int max_tasks);

    HlmTaskAddStatus addTask(shared_ptr<HlmTask> task, const string& streamUrl, const string& method);
    bool removeTask(const string& streamUrl, const string& method);
    void taskCompleted(shared_ptr<HlmTask> task, const string& task_key);

   private:
    void executeTask(shared_ptr<HlmTask> task, const string& task_key);
    void stopTask(shared_ptr<HlmTask> task);
    string createTaskKey(const string& streamUrl, const string& method);

    deque<shared_ptr<HlmTask>> task_queue_;
    vector<shared_ptr<HlmTask>> active_tasks_;
    unordered_map<string, bool> active_task_keys_;

    int max_tasks_;
    mutex mutex_;
};

#endif  // HLM_TASK_H

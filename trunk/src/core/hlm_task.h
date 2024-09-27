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

#include "hlm_mix_executor.h"

using namespace std;

// 基类 HlmTask
class HlmTask {
   public:
    enum class TaskType { Screenshot,
                          Recording,
                          Mixing };

    HlmTask(TaskType type, const string& stream_url, const string& method);
    virtual ~HlmTask() = default;

    TaskType getType() const;
    const string& getStreamUrl() const;
    const string& getMethod() const;
    void setCancelled(bool cancelled);
    bool isCancelled() const;

    virtual void execute() = 0;
    virtual void stop() = 0;

   private:
    TaskType type_;
    string stream_url_;
    string method_;
    bool cancelled_;
};

namespace HlmTaskAction {
const string Start = "start";
const string Stop = "stop";
const string Update = "update";
}  // namespace HlmTaskAction

enum class HlmTaskAddStatus {
    TaskAlreadyRunning,  // 已有相同任务在执行
    TaskQueued,          // 任务已加入队列
    TaskStarted,         // 任务已启动
    TaskUpdated,         // 任务已更新
    QueueFull,           // 队列已满，无法加入
    TaskNotFound         // 任务不存在
};

// HlmTaskManager 管理任务队列
class HlmTaskManager {
   public:
    HlmTaskManager(int max_tasks);

    HlmTaskAddStatus addTask(shared_ptr<HlmTask> task, const string& streamUrl, const string& method);
    HlmTaskAddStatus updateTask(const string& streamUrl, const string& method, const HlmMixTaskParams& params);
    bool removeTask(const string& streamUrl, const string& method);
    void taskCompleted(shared_ptr<HlmTask> task, const string& task_key);

   private:
    void executeTask(shared_ptr<HlmTask> task, const string& task_key);
    void updateMixTask(shared_ptr<HlmTask> task, const HlmMixTaskParams& params);
    void stopTask(shared_ptr<HlmTask> task);
    string createTaskKey(const string& streamUrl, const string& method);

    deque<shared_ptr<HlmTask>> task_queue_;
    vector<shared_ptr<HlmTask>> active_tasks_;
    unordered_map<string, bool> active_task_keys_;

    int max_tasks_;
    mutex mutex_;
};

#endif  // HLM_TASK_H

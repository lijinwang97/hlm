#include "hlm_task.h"

#include <algorithm>

#include "utils/hlm_logger.h"

HlmTask::HlmTask(TaskType type, const std::string& stream_url, const std::string& method)
    : type_(type), stream_url_(stream_url), method_(method) {}

HlmTask::TaskType HlmTask::getType() const { return type_; }

const std::string& HlmTask::getStreamUrl() const { return stream_url_; }

const std::string& HlmTask::getMethod() const { return method_; }

void HlmTask::setCancelled(bool cancelled) { cancelled_ = cancelled; }

bool HlmTask::isCancelled() const { return cancelled_; }

HlmMixingTask::HlmMixingTask(const string& input1, const string& input2, const string& output)
    : HlmTask(TaskType::Mixing, "stream_url", "method"), input1_(input1), input2_(input2) {}

void HlmMixingTask::execute() {
    cout << "Executing Mixing HlmTask with input1: " << input1_ << " and input2: " << input2_ << "\n";
}

HlmTaskManager::HlmTaskManager(int maxConcurrentTasks) : max_tasks_(maxConcurrentTasks) {}

HlmTaskAddStatus HlmTaskManager::addTask(shared_ptr<HlmTask> task, const string& stream_url, const string& method) {
    lock_guard<mutex> lock(mutex_);
    string task_key = createTaskKey(stream_url, method);
    if (active_task_keys_.count(task_key) > 0) {
        return HlmTaskAddStatus::TaskAlreadyRunning;
    }

    if (active_tasks_.size() >= max_tasks_) {
        task_queue_.push_back(task);
        hlm_info("Task queued. Current active tasks: {}/{}. Queued tasks: {}", active_tasks_.size(), max_tasks_, task_queue_.size());
        return HlmTaskAddStatus::TaskQueued;
    } else {
        active_tasks_.push_back(task);
        active_task_keys_[task_key] = true;
        hlm_info("Task started for stream_url: {}, method: {}. Active tasks: {}/{}", stream_url, method, active_tasks_.size(), max_tasks_);
        executeTask(task, task_key);
        return HlmTaskAddStatus::TaskStarted;
    }
}

bool HlmTaskManager::removeTask(const string& streamUrl, const string& method) {
    std::lock_guard<std::mutex> lock(mutex_);
    string task_key = createTaskKey(streamUrl, method);

    auto it = std::find_if(active_tasks_.begin(), active_tasks_.end(), [&](const shared_ptr<HlmTask>& task) {
        return task->getStreamUrl() == streamUrl && task->getMethod() == method;
    });

    if (it != active_tasks_.end()) {
        stopTask(*it);
        (*it)->setCancelled(true);
        active_tasks_.erase(it);
        active_task_keys_.erase(task_key);
        hlm_info("Task removed for stream_url: {}, method: {}. Active tasks: {}/{}", streamUrl, method, active_tasks_.size(), max_tasks_);
        return true;
    }

    auto queue_it = std::find_if(task_queue_.begin(), task_queue_.end(), [&](const shared_ptr<HlmTask>& task) {
        return task->getStreamUrl() == streamUrl && task->getMethod() == method;
    });

    if (queue_it != task_queue_.end()) {
        task_queue_.erase(queue_it);
        hlm_info("Queued task removed for stream_url: {}, method: {}. Queued tasks: {}", streamUrl, method, task_queue_.size());
        return true;
    }

    hlm_info("No active task found for stream_url: {}, method: {}", streamUrl, method);
    return false;
}

void HlmTaskManager::taskCompleted(shared_ptr<HlmTask> task, const string& task_key) {
    lock_guard<mutex> lock(mutex_);

    if (task->isCancelled()) {
        hlm_info("Task was cancelled for key: {}. Skipping completion.", task_key);
        return;
    }

    stopTask(task);
    active_tasks_.erase(remove(active_tasks_.begin(), active_tasks_.end(), task), active_tasks_.end());
    active_task_keys_.erase(task_key);
    hlm_info("Task completed for key: {}. Active tasks: {}/{}", task_key, active_tasks_.size(), max_tasks_);

    if (!task_queue_.empty()) {
        auto next_task = task_queue_.front();
        task_queue_.pop_front();

        string next_task_key = createTaskKey(next_task->getStreamUrl(), next_task->getMethod());
        active_tasks_.push_back(next_task);
        active_task_keys_[next_task_key] = true;
        hlm_info("Next task started for key: {}. Active tasks: {}/{}", next_task_key, active_tasks_.size(), max_tasks_);
        executeTask(next_task, next_task_key);
    }
}

void HlmTaskManager::executeTask(shared_ptr<HlmTask> task, const string& task_key) {
    thread([this, task, task_key]() {
        task->execute();
        taskCompleted(task, task_key);
    }).detach();
}

void HlmTaskManager::stopTask(shared_ptr<HlmTask> task) {
    task->stop();
    hlm_info("Stopped {} task for stream_url: {}, method: {}", task->getMethod(), task->getStreamUrl(), task->getMethod());
}

string HlmTaskManager::createTaskKey(const string& stream_url, const string& method) {
    return stream_url + "_" + method;
}
#include "hlm_task.h"

#include <algorithm>

HlmTask::HlmTask(TaskType type) : type_(type) {}

HlmTask::TaskType HlmTask::getType() const { return type_; }

HlmScreenshotTask::HlmScreenshotTask(const string& streamUrl, const string& outputDir, const string& filenamePrefix, int interval)
    : HlmTask(TaskType::Screenshot), stream_url_(streamUrl), output_dir_(outputDir), filename_prefix_(filenamePrefix), interval_(interval) {}

void HlmScreenshotTask::execute() {
    cout << "Executing Screenshot Task (interval) with URL: " << stream_url_ << " and interval: " << interval_ << " seconds.\n";
}

HlmPercentageScreenshotTask::HlmPercentageScreenshotTask(const string& streamUrl, const string& outputDir, const string& filenamePrefix, int percentage)
    : HlmTask(TaskType::Screenshot), stream_url_(streamUrl), output_dir_(outputDir), filename_prefix_(filenamePrefix), percentage_(percentage) {}

void HlmPercentageScreenshotTask::execute() {
    cout << "Executing Screenshot Task (percentage) with URL: " << stream_url_ << " and percentage: " << percentage_ << "%.\n";
}

HlmImmediateScreenshotTask::HlmImmediateScreenshotTask(const string& streamUrl, const string& outputDir, const string& filenamePrefix)
    : HlmTask(TaskType::Screenshot), stream_url_(streamUrl), output_dir_(outputDir), filename_prefix_(filenamePrefix) {}

void HlmImmediateScreenshotTask::execute() {
    cout << "Executing Immediate Screenshot Task with URL: " << stream_url_ << ".\n";
}

HlmSpecificTimeScreenshotTask::HlmSpecificTimeScreenshotTask(const string& streamUrl, const string& outputDir, const string& filenamePrefix, int timeSecond)
    : HlmTask(TaskType::Screenshot), stream_url_(streamUrl), output_dir_(outputDir), filename_prefix_(filenamePrefix), timeSecond_(timeSecond) {}

void HlmSpecificTimeScreenshotTask::execute() {
    cout << "Executing Specific Time Screenshot Task with URL: " << stream_url_ << " at time: " << timeSecond_ << " seconds.\n";
}

HlmRecordingTask::HlmRecordingTask(const string& streamUrl, const string& outputPath, int duration)
    : HlmTask(TaskType::Recording), stream_url_(streamUrl), outputPath_(outputPath), duration_(duration) {}

void HlmRecordingTask::execute() {
    cout << "Executing Recording HlmTask with URL: " << stream_url_ << " and duration: " << duration_ << "\n";
}

HlmMixingTask::HlmMixingTask(const string& input1, const string& input2, const string& output)
    : HlmTask(TaskType::Mixing), input1_(input1), input2_(input2), output_(output) {}

void HlmMixingTask::execute() {
    cout << "Executing Mixing HlmTask with input1: " << input1_ << " and input2: " << input2_ << "\n";
}

HlmTaskManager::HlmTaskManager(int maxConcurrentTasks) : max_tasks_(maxConcurrentTasks) {}

bool HlmTaskManager::addTask(shared_ptr<HlmTask> task) {
    lock_guard<mutex> lock(mutex_);
    if (active_tasks_.size() >= max_tasks_) {
        task_queue_.push(task);
        return false; 
    } else {
        active_tasks_.push_back(task);
        executeTask(task);          
        return true; 
    }
}

void HlmTaskManager::taskCompleted(shared_ptr<HlmTask> task) {
    lock_guard<mutex> lock(mutex_);
    active_tasks_.erase(remove(active_tasks_.begin(), active_tasks_.end(), task), active_tasks_.end());

    if (!task_queue_.empty()) {
        auto nextTask = task_queue_.front();
        task_queue_.pop();
        active_tasks_.push_back(nextTask);
        executeTask(nextTask);
    }
}

void HlmTaskManager::executeTask(shared_ptr<HlmTask> task) {
    thread([this, task]() {
        task->execute();

        // 模拟任务执行时间
        this_thread::sleep_for(chrono::seconds(5));

        taskCompleted(task);
    }).detach();
}

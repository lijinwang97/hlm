#include "hlm_task.h"

#include <algorithm>

// HlmTask 类构造函数
HlmTask::HlmTask(TaskType type) : type_(type) {}

// 获取任务类型
HlmTask::TaskType HlmTask::getType() const { return type_; }

// HlmScreenshotTask 实现
HlmScreenshotTask::HlmScreenshotTask(const string& streamUrl, const string& outputDir, const string& filenamePrefix, int interval)
    : HlmTask(TaskType::Screenshot), stream_url_(streamUrl), output_dir_(outputDir), filename_prefix_(filenamePrefix), interval_(interval) {}

void HlmScreenshotTask::execute() {
    cout << "Executing Screenshot Task (interval) with URL: " << stream_url_ << " and interval: " << interval_ << " seconds.\n";
    // 实际截图逻辑
}

// HlmPercentageScreenshotTask 实现
HlmPercentageScreenshotTask::HlmPercentageScreenshotTask(const string& streamUrl, const string& outputDir, const string& filenamePrefix, int percentage)
    : HlmTask(TaskType::Screenshot), stream_url_(streamUrl), output_dir_(outputDir), filename_prefix_(filenamePrefix), percentage_(percentage) {}

void HlmPercentageScreenshotTask::execute() {
    cout << "Executing Screenshot Task (percentage) with URL: " << stream_url_ << " and percentage: " << percentage_ << "%.\n";
    // 实际截图逻辑
}

// HlmImmediateScreenshotTask 实现
HlmImmediateScreenshotTask::HlmImmediateScreenshotTask(const string& streamUrl, const string& outputDir, const string& filenamePrefix)
    : HlmTask(TaskType::Screenshot), stream_url_(streamUrl), output_dir_(outputDir), filename_prefix_(filenamePrefix) {}

void HlmImmediateScreenshotTask::execute() {
    cout << "Executing Immediate Screenshot Task with URL: " << stream_url_ << ".\n";
    // 实际截图逻辑
}

// HlmSpecificTimeScreenshotTask 实现
HlmSpecificTimeScreenshotTask::HlmSpecificTimeScreenshotTask(const string& streamUrl, const string& outputDir, const string& filenamePrefix, int timeSecond)
    : HlmTask(TaskType::Screenshot), stream_url_(streamUrl), output_dir_(outputDir), filename_prefix_(filenamePrefix), timeSecond_(timeSecond) {}

void HlmSpecificTimeScreenshotTask::execute() {
    cout << "Executing Specific Time Screenshot Task with URL: " << stream_url_ << " at time: " << timeSecond_ << " seconds.\n";
    // 实际截图逻辑
}

// HlmRecordingTask 实现
HlmRecordingTask::HlmRecordingTask(const string& streamUrl, const string& outputPath, int duration)
    : HlmTask(TaskType::Recording), stream_url_(streamUrl), outputPath_(outputPath), duration_(duration) {}

void HlmRecordingTask::execute() {
    cout << "Executing Recording HlmTask with URL: " << stream_url_ << " and duration: " << duration_ << "\n";
    // 实际的录制逻辑实现
}

// HlmMixingTask 实现
HlmMixingTask::HlmMixingTask(const string& input1, const string& input2, const string& output)
    : HlmTask(TaskType::Mixing), input1_(input1), input2_(input2), output_(output) {}

void HlmMixingTask::execute() {
    cout << "Executing Mixing HlmTask with input1: " << input1_ << " and input2: " << input2_ << "\n";
    // 实际的混流逻辑实现
}

// HlmTaskManager 实现
HlmTaskManager::HlmTaskManager(int maxConcurrentTasks) : max_tasks_(maxConcurrentTasks) {}

bool HlmTaskManager::addTask(shared_ptr<HlmTask> task) {
    lock_guard<mutex> lock(mutex_);
    if (active_tasks_.size() >= max_tasks_) {
        task_queue_.push(task);
        return false; 
    } else {
        active_tasks_.push_back(task);  // 立即执行
        executeTask(task);             // 开始执行任务
        return true;                   // 任务立即执行
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
        task->execute();  // 根据任务类型执行对应的逻辑

        // 模拟任务执行时间
        this_thread::sleep_for(chrono::seconds(5));

        taskCompleted(task);
    }).detach();
}

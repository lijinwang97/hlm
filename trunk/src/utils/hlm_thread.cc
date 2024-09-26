#include "hlm_thread.h"
#include "hlm_logger.h"

HlmThread::HlmThread(const std::string& name, std::function<void()> process_func)
    : thread_name_(name), process_func_(process_func), running_(true) {}

void HlmThread::start() {
    thread_ = std::thread(&HlmThread::run, this);
}

void HlmThread::stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

bool HlmThread::isRunning() const {
    return running_;
}

void HlmThread::run() {
    hlm_info("Starting {} thread", thread_name_);
    process_func_();
    hlm_info("{} thread stopped", thread_name_);
}

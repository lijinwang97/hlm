#include "hlm_mix_task.h"

#include "hlm_mix_executor.h"
#include "utils/hlm_logger.h"

HlmMixTask::HlmMixTask(const HlmMixTaskParams& params)
    : HlmTask(TaskType::Mixing, params.output_url, HlmMixMethod::Mix), params_(params) {}

void HlmMixTask::execute() {
    if (!executor_) {
        executor_ = createExecutor();
    }
    executor_->execute();
}

void HlmMixTask::update(const HlmMixTaskParams& params) {
    if (executor_) {
        executor_->updateStreams(params);
    }
}

void HlmMixTask::stop() {
    if (executor_ && executor_->isRunning()) {
        executor_->stop();
    }
}

HlmDefaultMixTask::HlmDefaultMixTask(const HlmMixTaskParams& params)
    : HlmMixTask(params) {}

unique_ptr<HlmMixExecutor> HlmDefaultMixTask::createExecutor() {
    return make_unique<HlmDefaultMixExecutor>(params_);
}

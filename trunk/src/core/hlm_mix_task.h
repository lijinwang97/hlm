#ifndef HLM_MIX_TASK_H
#define HLM_MIX_TASK_H

#include <memory>
#include <string>
#include <vector>

#include "hlm_task.h"
#include "hlm_mix_executor.h"

class HlmMixTask : public HlmTask {
   public:
    HlmMixTask( const HlmMixTaskParams& params);

    void execute() override;
    void update(const HlmMixTaskParams& params);
    void stop() override;

   protected:
    virtual unique_ptr<HlmMixExecutor> createExecutor() = 0;

    unique_ptr<HlmMixExecutor> executor_;
    HlmMixTaskParams params_;
};

// 默认混流任务实现
class HlmDefaultMixTask : public HlmMixTask {
   public:
    HlmDefaultMixTask(const HlmMixTaskParams& params);

   protected:
    unique_ptr<HlmMixExecutor> createExecutor() override;
};

#endif  // HLM_MIX_TASK_H

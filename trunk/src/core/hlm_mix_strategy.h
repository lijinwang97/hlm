#ifndef HLM_MIX_STRATEGY_H
#define HLM_MIX_STRATEGY_H

#include <memory>
#include <vector>
#include <string>
#include "hlm_mix_task.h"
#include "hlm_task.h"

class HlmMixStrategy {
public:
    virtual ~HlmMixStrategy() = default;

    virtual shared_ptr<HlmTask> createTask(const HlmMixTaskParams& params) = 0;
};

class HlmMixStrategyFactory {
public:
    static shared_ptr<HlmMixStrategy> createStrategy(const string& strategy_name);
};

class HlmDefaultMixStrategy : public HlmMixStrategy {
public:
    shared_ptr<HlmTask> createTask(const HlmMixTaskParams& params) override;
};

#endif  // HLM_MIX_STRATEGY_H

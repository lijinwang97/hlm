#include "hlm_mix_strategy.h"

#include <stdexcept>

#include "hlm_executor.h"
#include "utils/hlm_logger.h"

shared_ptr<HlmMixStrategy> HlmMixStrategyFactory::createStrategy(const string& strategy_name) {
    if (strategy_name == HlmMixMethod::Mix) {
        return make_shared<HlmDefaultMixStrategy>();
    } else {
        throw invalid_argument("Invalid mixing strategy name.");
    }
}

shared_ptr<HlmTask> HlmDefaultMixStrategy::createTask(const HlmMixTaskParams& params) {
    return make_shared<HlmDefaultMixTask>(params);
}

#ifndef HLM_RECORDING_STRATEGY_H
#define HLM_RECORDING_STRATEGY_H

#include <memory>
#include <stdexcept>
#include <string>

#include "crow.h"
#include "hlm_task.h"

using namespace crow;
using namespace std;

// 策略基类 HlmRecordingStrategy
class HlmRecordingStrategy {
   public:
    virtual ~HlmRecordingStrategy() = default;
    virtual shared_ptr<HlmTask> createTask(const string& stream_url, const string& method, const string& output_dir, const string& filename, const json::rvalue& body) = 0;
};

// 录制MP4策略
class HlmMp4RecordingStrategy : public HlmRecordingStrategy {
   public:
    shared_ptr<HlmTask> createTask(const string& stream_url, const string& method, const string& output_dir, const string& filename, const json::rvalue& body) override;
};

// 录制HLS策略
class HlmHlsRecordingStrategy : public HlmRecordingStrategy {
   public:
    shared_ptr<HlmTask> createTask(const string& stream_url, const string& method, const string& output_dir, const string& filename, const json::rvalue& body) override;
};

// 使用工厂创建策略
class HlmRecordingStrategyFactory {
   public:
    static shared_ptr<HlmRecordingStrategy> createStrategy(const string& method);
};

#endif  // HLM_RECORDING_STRATEGY_H

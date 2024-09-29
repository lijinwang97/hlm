#ifndef HLM_MIX_EXECUTOR_H
#define HLM_MIX_EXECUTOR_H

#include <string>
#include <unordered_map>

#include "hlm_executor.h"

using namespace std;

struct HlmResolution {
    int width;
    int height;
};

struct HlmStreamInfo {
    string id;
    string url;
    int width;
    int height;
    int x;
    int y;
    int z_index;
};

struct HlmMixTaskParams {
    string background_image;
    string output_url;
    HlmResolution resolution;
    vector<HlmStreamInfo> streams;
};

class HlmMixExecutor : public HlmExecutor {
   public:
    HlmMixExecutor(const HlmMixTaskParams& params);
    virtual ~HlmMixExecutor() = default;

    bool init() override;
    bool initOutputFile() override;
    void execute() override;

    bool loadBackgroundImage(const string& image_url);
    void updateStreams(const HlmMixTaskParams& params);

   protected:
    void endMixing();

   protected:
    HlmMixTaskParams params_;
    unordered_map<string, HlmStreamInfo> active_streams_;
    AVPixelFormat target_pix_fmt_;
    AVFrame* background_frame_ = nullptr;
    mutex mix_mutex_;
};

class HlmDefaultMixExecutor : public HlmMixExecutor {
   public:
    HlmDefaultMixExecutor(const HlmMixTaskParams& params);
    void checkAndSavePacket(AVPacket* encoded_packet, int stream_index) override;
};

#endif  // HLM_MIX_EXECUTOR_H

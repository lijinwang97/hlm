#ifndef HLM_HTTP_SERVER_H
#define HLM_HTTP_SERVER_H

#include "core/hlm_executor.h"
#include "core/hlm_mix_strategy.h"
#include "core/hlm_recording_strategy.h"
#include "core/hlm_recording_task.h"
#include "core/hlm_screenshot_strategy.h"
#include "core/hlm_screenshot_task.h"
#include "core/hlm_task.h"
#include "crow.h"
#include "utils/hlm_logger.h"

using namespace crow;
using namespace std;

enum StatusCode {
    SUCCESS = 1000,
    QUEUED = 1001,
    INVALID_REQUEST = 2001,
    INVALID_JSON = 2002
};

struct ValidationResult {
    int code;
    string message;
};

class HlmHttpServer {
   public:
    HlmHttpServer(int max_tasks);
    void start(int port);
    void setLogLevel(LogLevel level);

   private:
    void initRoutes();

    response manageScreenshotReq(const request& req);
    response startScreenshot(const json::rvalue& body);
    response stopScreenshot(const json::rvalue& body);

    response manageRecordingReq(const request& req);
    response startRecording(const json::rvalue& body);
    response stopRecording(const json::rvalue& body);

    response manageMixReq(const request& req);
    response startMix(const json::rvalue& body);
    response updateMix(const json::rvalue& body);
    ValidationResult validateMixRequest(const json::rvalue& body);
    HlmMixTaskParams parseMixParams(const json::rvalue& body);
    vector<HlmStreamInfo> parseStreams(const json::rvalue& streams_json);

    // 装饰器模式：包装处理函数，添加日志功能
    response logWrapper(const request& req, function<response(const request&)> handler);

    response createJsonResponse(int code, const string& message);
    bool validateJson(const json::rvalue& body, const vector<string>& required_fields, map<string, string>& error_map);
    string getOrDefault(const json::rvalue& body, const string& field, const string& default_value);

   private:
    HlmTaskManager task_manager_;
    SimpleApp app_;
};

#endif  // HLM_HTTP_SERVER_H

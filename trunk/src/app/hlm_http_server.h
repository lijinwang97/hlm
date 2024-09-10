#ifndef HLM_HTTP_SERVER_H
#define HLM_HTTP_SERVER_H

#include "core/hlm_task.h"
#include "crow.h"

using namespace crow;
using namespace std;

enum StatusCode {
    SUCCESS = 1000,
    QUEUED = 1001,
    INVALID_REQUEST = 2001,
    INVALID_JSON = 2002
};

class HlmHttpServer {
   public:
    HlmHttpServer(int max_tasks);
    void start(int port);
    void setLogLevel(LogLevel level);

   private:
    void initRoutes();

    response handleScreenshot(const request& req);
    response handleRecording(const request& req);
    response handleMix(const request& req);

    // 装饰器模式：包装处理函数，添加日志功能
    response logWrapper(const request& req, function<response(const request&)> handler);

    response createJsonResponse(int code, const string& message);

   private:
    HlmTaskManager task_manager_;
    SimpleApp app_;
};

#endif  // HLM_HTTP_SERVER_H

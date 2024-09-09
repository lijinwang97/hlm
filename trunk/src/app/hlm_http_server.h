#ifndef HLM_HTTP_SERVER_H
#define HLM_HTTP_SERVER_H

#include "crow.h"

using namespace crow;

class HlmHttpServer {
public:
    HlmHttpServer();
    void start(int port);
    void setLogLevel(crow::LogLevel level);

private:
    void initRoutes();

    response handleScreenshot(const request& req);
    response handleRecording(const request& req);
    response handleMix(const request& req);

    crow::SimpleApp app_;
};

#endif // HLM_HTTP_SERVER_H

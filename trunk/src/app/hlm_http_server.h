#ifndef HLM_HTTP_SERVER_H
#define HLM_HTTP_SERVER_H

#include "crow.h"

using namespace crow;

class HlmHttpServer {
public:
    HlmHttpServer();
    void Start(int port);
    void SetLogLevel(crow::LogLevel level);

private:
    void InitRoutes();

    response HandleScreenshot(const request& req);
    response HandleRecording(const request& req);
    response HandleMix(const request& req);

    crow::SimpleApp app_;
};

#endif // HLM_HTTP_SERVER_H

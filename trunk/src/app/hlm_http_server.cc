#include "hlm_http_server.h"

#include <iostream>
#include <string>

#include "core/hlm_screenshot_strategy.h"
#include "utils/hlm_logger.h"

HlmHttpServer::HlmHttpServer(int max_tasks) : task_manager_(max_tasks) {
    initRoutes();
    setLogLevel(LogLevel::Warning);
}

void HlmHttpServer::initRoutes() {
    CROW_ROUTE(app_, "/screenshot").methods(HTTPMethod::Post)([this](const request& req) {
        return logWrapper(req, [this](const request& req) {
            return handleScreenshot(req);
        });
    });

    CROW_ROUTE(app_, "/record").methods(HTTPMethod::Post)([this](const request& req) {
        return logWrapper(req, [this](const request& req) {
            return handleRecording(req);
        });
    });

    CROW_ROUTE(app_, "/mix").methods(HTTPMethod::Post)([this](const request& req) {
        return logWrapper(req, [this](const request& req) {
            return handleMix(req);
        });
    });
}

void HlmHttpServer::start(int port) {
    hlm_info("Starting HlmHttpServer on port: {}", port);
    app_.port(port).multithreaded().run();
}

void HlmHttpServer::setLogLevel(LogLevel level) {
    app_.loglevel(level);
}

response HlmHttpServer::handleScreenshot(const request& req) {
    auto body = json::load(req.body);
    if (!body) {
        return createJsonResponse(INVALID_JSON, "Invalid JSON");
    }

    if (!body.has("stream_url")) {
        return createJsonResponse(INVALID_REQUEST, "Missing stream_url");
    }

    string streamUrl = body["stream_url"].s();
    string outputDir = body.has("output_dir") ? body["output_dir"].s() : streamUrl.substr(streamUrl.find_last_of('/') + 1);
    string filenamePrefix = body.has("filename_prefix") ? body["filename_prefix"].s() : string("snapshot");

    if (!body.has("method")) {
        return createJsonResponse(INVALID_REQUEST, "Missing method");
    }
    string method = body["method"].s();

    try {
        auto strategy = HlmScreenshotStrategyFactory::createStrategy(method);
        auto task = strategy->createTask(streamUrl, outputDir, filenamePrefix, body);
        if (task_manager_.addTask(task)) {
            return createJsonResponse(SUCCESS, "Screenshot task started");
        } else {
            return createJsonResponse(QUEUED, "Task queued, waiting for execution");
        }
    } catch (const invalid_argument& e) {
        return createJsonResponse(INVALID_REQUEST, e.what());
    }
}

response HlmHttpServer::handleRecording(const request& req) {
    auto body = json::load(req.body);
    if (!body) {
        return response(400, "Invalid JSON");
    }

    int duration = body["duration"].i();
    string output_file = body["output_file"].s();

    hlm_info("handleRecording duration:{} output_file:{} output:{}", duration, output_file);
    // start_recording(output_file, duration);

    return response(200, "Recording started!");
}

response HlmHttpServer::handleMix(const request& req) {
    auto body = json::load(req.body);
    if (!body) {
        return response(400, "Invalid JSON");
    }

    string input1 = body["input1"].s();
    string input2 = body["input2"].s();
    string output = body["output"].s();

    hlm_info("handleMix input1:{} input2:{} output:{}", input1, input2, output);
    // mix_streams(input1, input2, output);

    return response(200, "Mixing successful!");
}

response HlmHttpServer::logWrapper(const request& req, function<response(const request&)> handler) {
    hlm_info("Received request url:{}, Body:{}", req.url, req.body);
    response res = handler(req);
    hlm_info("Response url:{}, Body:{}", req.url, res.body);
    return res;
}

response HlmHttpServer::createJsonResponse(int code, const string& message) {
    json::wvalue jsonResp;
    jsonResp["code"] = code;
    jsonResp["message"] = message;
    return response(jsonResp);
}
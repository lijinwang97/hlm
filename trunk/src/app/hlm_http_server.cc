#include "hlm_http_server.h"

#include <iostream>
#include <string>

#include "core/hlm_recording_strategy.h"
#include "core/hlm_recording_task.h"
#include "core/hlm_screenshot_strategy.h"
#include "core/hlm_screenshot_task.h"
#include "utils/hlm_logger.h"

HlmHttpServer::HlmHttpServer(int max_tasks) : task_manager_(max_tasks) {
    initRoutes();
    setLogLevel(LogLevel::Warning);
}

void HlmHttpServer::initRoutes() {
    CROW_ROUTE(app_, "/screenshot").methods(HTTPMethod::Post)([this](const request& req) {
        return logWrapper(req, [this](const request& req) {
            return manageScreenshotReq(req);
        });
    });

    CROW_ROUTE(app_, "/recording").methods(HTTPMethod::Post)([this](const request& req) {
        return logWrapper(req, [this](const request& req) {
            return manageRecordingReq(req);
        });
    });

    CROW_ROUTE(app_, "/mix").methods(HTTPMethod::Post)([this](const request& req) {
        return logWrapper(req, [this](const request& req) {
            return manageMixReq(req);
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

response HlmHttpServer::manageScreenshotReq(const request& req) {
    auto body = json::load(req.body);
    if (!body) {
        return createJsonResponse(INVALID_JSON, "Invalid JSON");
    }

    std::map<std::string, std::string> errors;
    std::vector<std::string> required_fields = {"stream_url", "method", "action"};
    if (!validateJson(body, required_fields, errors)) {
        return createJsonResponse(INVALID_REQUEST, "Missing fields: " + errors.begin()->second);
    }

    string action = body["action"].s();
    if (action == HlmTaskAction::Start) {
        return startScreenshot(body);
    } else if (action == HlmTaskAction::Stop) {
        return stopScreenshot(body);
    } else {
        return createJsonResponse(INVALID_REQUEST, "Invalid action. Valid actions are 'start' or 'stop'.");
    }
}

response HlmHttpServer::startScreenshot(const json::rvalue& body) {
    string stream_url = body["stream_url"].s();
    string method = body["method"].s();
    string output_dir = getOrDefault(body, "output_dir", stream_url.substr(stream_url.find_last_of('/') + 1));
    string filename_prefix = getOrDefault(body, "filename_prefix", stream_url.substr(stream_url.find_last_of('/') + 1));

    // 按时间间隔截图(interval)方式：支持实时流和文件
    // 立即截图(immediate)方式：只支持实时流
    // 按百分比截图(percentage)和指定时间点截图(specific_time)方式：只支持文件
    bool isRtmpStream = (stream_url.find("rtmp://") == 0);
    if (method == HlmScreenshotMethod::Percentage || method == HlmScreenshotMethod::SpecificTime) {
        if (isRtmpStream) {
            return createJsonResponse(INVALID_REQUEST, "Percentage and specific time screenshot are not supported for streams.");
        }
    } else if (method == HlmScreenshotMethod::Immediate) {
        if (!isRtmpStream) {
            return createJsonResponse(INVALID_REQUEST, "Immediate screenshot is only supported for streams.");
        }
    }

    try {
        auto strategy = HlmScreenshotStrategyFactory::createStrategy(method);
        auto task = strategy->createTask(stream_url, method, output_dir, filename_prefix, body);
        HlmTaskAddStatus status = task_manager_.addTask(task, stream_url, method);
        switch (status) {
            case HlmTaskAddStatus::TaskAlreadyRunning:
                return createJsonResponse(INVALID_REQUEST, "A screenshot task with the same stream URL and method is already running.");
            case HlmTaskAddStatus::TaskQueued:
                return createJsonResponse(QUEUED, "Task queued, waiting for execution.");
            case HlmTaskAddStatus::TaskStarted:
                return createJsonResponse(SUCCESS, "Screenshot task started.");
            case HlmTaskAddStatus::QueueFull:
                return createJsonResponse(INVALID_REQUEST, "Task queue is full, unable to add task.");
        }
    } catch (const invalid_argument& e) {
        return createJsonResponse(INVALID_REQUEST, e.what());
    }

    return createJsonResponse(SUCCESS, "Screenshot started successfully");
}

response HlmHttpServer::stopScreenshot(const json::rvalue& body) {
    string stream_url = body["stream_url"].s();
    string method = body["method"].s();

    if (task_manager_.removeTask(stream_url, method)) {
        return createJsonResponse(SUCCESS, "Screenshot task stopped successfully.");
    } else {
        return createJsonResponse(INVALID_REQUEST, "No running screenshot task found for this stream.");
    }
}

response HlmHttpServer::manageRecordingReq(const request& req) {
    auto body = json::load(req.body);
    if (!body) {
        return createJsonResponse(INVALID_JSON, "Invalid JSON");
    }

    std::map<std::string, std::string> errors;
    std::vector<std::string> required_fields = {"stream_url", "method", "action"};
    if (!validateJson(body, required_fields, errors)) {
        return createJsonResponse(INVALID_REQUEST, "Missing fields: " + errors.begin()->second);
    }

    string action = body["action"].s();
    if (action == HlmTaskAction::Start) {
        return startRecording(body);
    } else if (action == HlmTaskAction::Stop) {
        return stopRecording(body);
    } else {
        return createJsonResponse(INVALID_REQUEST, "Invalid action. Valid actions are 'start' or 'stop'.");
    }
}

response HlmHttpServer::startRecording(const json::rvalue& body) {
    string stream_url = body["stream_url"].s();
    string method = body["method"].s();
    string output_dir = getOrDefault(body, "output_dir", stream_url.substr(stream_url.find_last_of('/') + 1));
    string filename_prefix = getOrDefault(body, "filename_name", stream_url.substr(stream_url.find_last_of('/') + 1));

    // 录制成HLS和MP4：只支持实时流
    bool isRtmpStream = (stream_url.find("rtmp://") == 0);
    if (!isRtmpStream) {
        return createJsonResponse(INVALID_REQUEST, "Recording is only supported for streams.");
    }

    try {
        auto strategy = HlmRecordingStrategyFactory::createStrategy(method);
        auto task = strategy->createTask(stream_url, method, output_dir, filename_prefix, body);
        HlmTaskAddStatus status = task_manager_.addTask(task, stream_url, method);
        switch (status) {
            case HlmTaskAddStatus::TaskAlreadyRunning:
                return createJsonResponse(INVALID_REQUEST, "A recording task with the same stream URL and method is already running.");
            case HlmTaskAddStatus::TaskQueued:
                return createJsonResponse(QUEUED, "Task queued, waiting for execution.");
            case HlmTaskAddStatus::TaskStarted:
                return createJsonResponse(SUCCESS, "Recording task started.");
            case HlmTaskAddStatus::QueueFull:
                return createJsonResponse(INVALID_REQUEST, "Task queue is full, unable to add task.");
        }
    } catch (const invalid_argument& e) {
        return createJsonResponse(INVALID_REQUEST, e.what());
    }

    return createJsonResponse(SUCCESS, "Recording started successfully");
}

response HlmHttpServer::stopRecording(const json::rvalue& body) {
    string stream_url = body["stream_url"].s();
    string method = body["method"].s();

    if (task_manager_.removeTask(stream_url, method)) {
        return createJsonResponse(SUCCESS, "Recording task stopped successfully.");
    } else {
        return createJsonResponse(INVALID_REQUEST, "No running recording task found for this stream.");
    }
}

response HlmHttpServer::manageMixReq(const request& req) {
    auto body = json::load(req.body);
    if (!body) {
        return response(400, "Invalid JSON");
    }

    string input1 = body["input1"].s();
    string input2 = body["input2"].s();
    string output = body["output"].s();

    hlm_info("manageMixReq input1:{} input2:{} output:{}", input1, input2, output);
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

bool HlmHttpServer::validateJson(const json::rvalue& body, const vector<string>& required_fields, map<string, string>& error_map) {
    for (const auto& field : required_fields) {
        if (!body.has(field)) {
            error_map[field] = "Missing " + field;
        }
    }
    return error_map.empty();
}

string HlmHttpServer::getOrDefault(const json::rvalue& body, const string& field, const string& default_value) {
    return body.has(field) ? body[field].s() : default_value;
}
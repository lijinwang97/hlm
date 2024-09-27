#include "hlm_http_server.h"

#include <iostream>
#include <string>

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

    map<string, string> errors;
    vector<string> required_fields = {"stream_url", "method", "action"};
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

    map<string, string> errors;
    vector<string> required_fields = {"stream_url", "method", "action"};
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
    string filename = body["filename_name"].s();

    if (!(filename.size() >= 4 && (filename.substr(filename.size() - 4) == ".mp4" || filename.substr(filename.size() - 5) == ".m3u8"))) {
        return createJsonResponse(INVALID_REQUEST, "Filename must end with .mp4 or .m3u8.");
    }

    // 录制成HLS和MP4：只支持实时流
    bool isRtmpStream = (stream_url.find("rtmp://") == 0);
    if (!isRtmpStream) {
        return createJsonResponse(INVALID_REQUEST, "Recording is only supported for streams.");
    }

    try {
        auto strategy = HlmRecordingStrategyFactory::createStrategy(method);
        auto task = strategy->createTask(stream_url, method, output_dir, filename, body);
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
        return createJsonResponse(INVALID_JSON, "Invalid JSON");
    }

    map<string, string> errors;
    vector<string> required_fields = {"output_url", "action", "streams"};
    if (!validateJson(body, required_fields, errors)) {
        return createJsonResponse(INVALID_REQUEST, "Missing fields: " + errors.begin()->second);
    }

    string action = body["action"].s();
    if (action == HlmTaskAction::Start) {
        return startMix(body);
    } else if (action == HlmTaskAction::Update) {
        return updateMix(body);
    } else {
        return createJsonResponse(INVALID_REQUEST, "Invalid action. Valid actions are 'start' or 'stop'.");
    }
}

response HlmHttpServer::startMix(const json::rvalue& body) {
    auto validation_result = validateMixRequest(body);
    if (validation_result.code != SUCCESS) {
        return createJsonResponse(validation_result.code, validation_result.message);
    }

    HlmMixTaskParams params = parseMixParams(body);
    if (params.output_url.empty()) {
        return createJsonResponse(INVALID_REQUEST, "Failed to parse mixing parameters.");
    }

    try {
        auto strategy = HlmMixStrategyFactory::createStrategy(HlmMixMethod::Mix);
        auto task = strategy->createTask(params);
        HlmTaskAddStatus status = task_manager_.addTask(task, params.output_url, HlmMixMethod::Mix);

        switch (status) {
            case HlmTaskAddStatus::TaskAlreadyRunning:
                return createJsonResponse(INVALID_REQUEST, "A mixing task with the same RTMP URL is already running.");
            case HlmTaskAddStatus::TaskQueued:
                return createJsonResponse(QUEUED, "Mixing task queued, waiting for execution.");
            case HlmTaskAddStatus::TaskStarted:
                return createJsonResponse(SUCCESS, "Mixing task started.");
            case HlmTaskAddStatus::QueueFull:
                return createJsonResponse(INVALID_REQUEST, "Task queue is full, unable to add task.");
        }
    } catch (const invalid_argument& e) {
        return createJsonResponse(INVALID_REQUEST, e.what());
    }

    return createJsonResponse(SUCCESS, "Mixing started successfully");
}

response HlmHttpServer::updateMix(const json::rvalue& body) {
    string output_url = body["output_url"].s();
    if (output_url.find("rtmp://") != 0) {
        return {INVALID_REQUEST, "Mixing is only supported for RTMP streams."};
    }

    vector<HlmStreamInfo> streams = parseStreams(body["streams"]);
    if (streams.empty()) {
        return createJsonResponse(INVALID_REQUEST, "Missing fields: 'streams'");
    }

    HlmMixTaskParams params = {"", output_url, {}, streams};
    


    return createJsonResponse(SUCCESS, "Mixing updated successfully");
}

ValidationResult HlmHttpServer::validateMixRequest(const json::rvalue& body) {
    if (!body.has("resolution")) {
        return {INVALID_REQUEST, "Missing required fields: 'resolution'."};
    }

    string output_url = body["output_url"].s();
    if (output_url.find("rtmp://") != 0) {
        return {INVALID_REQUEST, "Mixing is only supported for RTMP streams."};
    }

    auto resolution_json = body["resolution"];
    if (!resolution_json.has("width") || !resolution_json.has("height")) {
        return {INVALID_REQUEST, "Resolution requires both 'width' and 'height'."};
    }

    int width = resolution_json["width"].i();
    int height = resolution_json["height"].i();
    if (width <= 0 || height <= 0) {
        return {INVALID_REQUEST, "Resolution dimensions must be positive integers."};
    }

    return {SUCCESS, "Validation succeeded"};
}

HlmMixTaskParams HlmHttpServer::parseMixParams(const json::rvalue& body) {
    HlmResolution resolution;
    resolution.width = body["resolution"]["width"].i();
    resolution.height = body["resolution"]["height"].i();

    string background_image = body["background_image"].s();
    string output_url = body["output_url"].s();

    vector<HlmStreamInfo> streams = parseStreams(body["streams"]);
    if (streams.size() == 0) {
        return {};
    }

    return {background_image, output_url, resolution, streams};
}

vector<HlmStreamInfo> HlmHttpServer::parseStreams(const json::rvalue& streams_json) {
    vector<HlmStreamInfo> streams;
    for (const auto& stream : streams_json) {
        if (!stream.has("id") || !stream.has("url") || !stream.has("width") ||
            !stream.has("height") || !stream.has("x") || !stream.has("y") || !stream.has("z-index")) {
            continue;
        }

        HlmStreamInfo stream_info;
        stream_info.id = stream["id"].s();
        stream_info.url = stream["url"].s();
        stream_info.width = stream["width"].i();
        stream_info.height = stream["height"].i();
        stream_info.x = stream["x"].i();
        stream_info.y = stream["y"].i();
        stream_info.z_index = stream["z-index"].i();

        if (stream_info.url.find("rtmp://") != 0) {
            continue;
        }

        streams.push_back(stream_info);
    }

    return streams;
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
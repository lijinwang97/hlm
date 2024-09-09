#include "hlm_http_server.h"

#include <iostream>
#include <string>

#include "utils/hlm_design_patterns.h"
#include "utils/hlm_logger.h"

HlmHttpServer::HlmHttpServer() {
  InitRoutes();
  SetLogLevel(crow::LogLevel::Warning);
}

void HlmHttpServer::InitRoutes() {
  CROW_ROUTE(app_, "/screenshot").methods(HTTPMethod::Post)([this](const request& req) {
    return LogWrapper(req, [this](const request& req) {
      return HandleScreenshot(req);
    });
  });

  CROW_ROUTE(app_, "/record").methods(HTTPMethod::Post)([this](const request& req) {
    return LogWrapper(req, [this](const request& req) {
      return HandleRecording(req);
    });
  });

  CROW_ROUTE(app_, "/mix").methods(HTTPMethod::Post)([this](const request& req) {
    return LogWrapper(req, [this](const request& req) {
      return HandleMix(req);
    });
  });
}

void HlmHttpServer::Start(int port) {
  hlm_info("Starting HlmHttpServer on port: {}", port);
  app_.port(port).multithreaded().run();
}

void HlmHttpServer::SetLogLevel(crow::LogLevel level) {
  app_.loglevel(level);
}

response HlmHttpServer::HandleScreenshot(const request& req) {
  auto body = json::load(req.body);
  if (!body) {
    return response(400, "Invalid JSON");
  }

  std::string filename = body["filename"].s();
  int width = body["width"].i();
  int height = body["height"].i();

  hlm_info("HandleScreenshot filename:{} width:{} height:{}", filename, width, height);
  // take_screenshot(filename, width, height);

  return response(200, "Screenshot successful!");
}

response HlmHttpServer::HandleRecording(const request& req) {
  auto body = json::load(req.body);
  if (!body) {
    return response(400, "Invalid JSON");
  }

  int duration = body["duration"].i();
  std::string output_file = body["output_file"].s();

  hlm_info("HandleRecording duration:{} output_file:{} output:{}", duration, output_file);
  // start_recording(output_file, duration);

  return response(200, "Recording started!");
}

response HlmHttpServer::HandleMix(const request& req) {
  auto body = json::load(req.body);
  if (!body) {
    return response(400, "Invalid JSON");
  }

  std::string input1 = body["input1"].s();
  std::string input2 = body["input2"].s();
  std::string output = body["output"].s();

  hlm_info("HandleMix input1:{} input2:{} output:{}", input1, input2, output);
  // mix_streams(input1, input2, output);

  return response(200, "Mixing successful!");
}

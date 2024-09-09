#include "hlm_http_server.h"

#include <iostream>
#include <string>

#include "utils/hlm_design_patterns.h"
#include "utils/hlm_logger.h"

HlmHttpServer::HlmHttpServer() {
  initRoutes();
  setLogLevel(crow::LogLevel::Warning);
}

void HlmHttpServer::initRoutes() {
  CROW_ROUTE(app_, "/screenshot").methods(HTTPMethod::Post)([this](const request& req) {
    return LogWrapper(req, [this](const request& req) {
      return handleScreenshot(req);
    });
  });

  CROW_ROUTE(app_, "/record").methods(HTTPMethod::Post)([this](const request& req) {
    return LogWrapper(req, [this](const request& req) {
      return handleRecording(req);
    });
  });

  CROW_ROUTE(app_, "/mix").methods(HTTPMethod::Post)([this](const request& req) {
    return LogWrapper(req, [this](const request& req) {
      return handleMix(req);
    });
  });
}

void HlmHttpServer::start(int port) {
  hlm_info("Starting HlmHttpServer on port: {}", port);
  app_.port(port).multithreaded().run();
}

void HlmHttpServer::setLogLevel(crow::LogLevel level) {
  app_.loglevel(level);
}

response HlmHttpServer::handleScreenshot(const request& req) {
  auto body = json::load(req.body);
  if (!body) {
    return response(400, "Invalid JSON");
  }

  std::string filename = body["filename"].s();
  int width = body["width"].i();
  int height = body["height"].i();

  hlm_info("handleScreenshot filename:{} width:{} height:{}", filename, width, height);
  // take_screenshot(filename, width, height);

  return response(200, "Screenshot successful!");
}

response HlmHttpServer::handleRecording(const request& req) {
  auto body = json::load(req.body);
  if (!body) {
    return response(400, "Invalid JSON");
  }

  int duration = body["duration"].i();
  std::string output_file = body["output_file"].s();

  hlm_info("handleRecording duration:{} output_file:{} output:{}", duration, output_file);
  // start_recording(output_file, duration);

  return response(200, "Recording started!");
}

response HlmHttpServer::handleMix(const request& req) {
  auto body = json::load(req.body);
  if (!body) {
    return response(400, "Invalid JSON");
  }

  std::string input1 = body["input1"].s();
  std::string input2 = body["input2"].s();
  std::string output = body["output"].s();

  hlm_info("handleMix input1:{} input2:{} output:{}", input1, input2, output);
  // mix_streams(input1, input2, output);

  return response(200, "Mixing successful!");
}

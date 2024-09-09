#include <chrono>
#include <iostream>
#include <stdexcept>

#include "app/hlm_http_server.h"
#include "utils/hlm_config.h"
#include "utils/hlm_logger.h"

using namespace spdlog;

void init(const std::string& config_file) {
  CONF.load(config_file);
  Logger::init(CONF.getLogLevel(), CONF.getLogTarget(), CONF.getLogBaseName(),
               CONF.useAsyncLogging(), CONF.getLogMaxFileSize(), CONF.getLogMaxFiles());
  CONF.printAllConfigs();
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
    return 1;
  }
  init(argv[1]);

  HlmHttpServer server;
  server.start(CONF.getHttpPort());

  return 0;
}

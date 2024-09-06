#include <chrono>
#include <iostream>
#include <stdexcept>

#include "utils/Config.h"
#include "utils/Logger.h"

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

  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;
  hlm_info("Conversion took {} seconds.", duration.count());

  return 0;
}

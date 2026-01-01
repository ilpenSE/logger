#include <iostream>
#include <string>

#include "logger.h"
#include "loggerstream.hpp"

/**
This is NOT a JSON logger, its human-readable logger
Usage is provided below.

it no longer expects trailing slash, if its not provided, it should generate
if isLocalTime set to 1, it fetches your time but if you set it false it fetches UTC datetime
*/
int main(int argc, char** argv) {
  std::string path = "logs";
  int isLocalTime = 1;
  if (argc >= 3) {
    path = argv[1];
    isLocalTime = (std::string(argv[2]) == "1");
  }

  if (!logger_init(path.c_str(), isLocalTime)) {
    std::cout << "[MAIN] Logger init failed" << std::endl;
    return -1;
  }

  linfo << "Hello" << "World!";
  lerror << "Some error occured";
  lwarning << "Some warning";
  return 0;
}

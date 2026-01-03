#include "logger.h"
#include "logger.hpp"
#include <iostream>

int lg_init(const char* logs_dir, int use_local_time, int should_throw_error) {
  try {
    return Logger::instance().initialize(
      logs_dir ? logs_dir : "logs/",
      use_local_time != 0,
      should_throw_error != 0
    );
  } catch (const std::exception& e) {
    // swallow logger's runtime exceptions
    // if LOGGER_VERBOSE defined, print exception message to std::cerr
    // you can define it in compile options by using
    // -DLOGGER_VERBOSE or target_compile_definitions(logger PRIVATE LOGGER_VERBOSE)
#ifdef LOGGER_VERBOSE
    const char* msg = e.what();
    std::cerr << "Runtime Exception occured when initializing the Logger:\n";
    std::cerr << msg << '\n';
#endif
    return -1;
  }
}

int lg_info(const char* msg) {
  try {
    return Logger::instance().logInfo(msg);
  } catch(const std::exception& e) {
#ifdef LOGGER_VERBOSE
    const char* msg = e.what();
    std::cerr << "Runtime Exception occured when logging:\n";
    std::cerr << msg << '\n';
#endif
    return -1;
  }
}

int lg_error(const char* msg) {
  try {
    return Logger::instance().logError(msg);
  } catch(const std::exception& e) {
#ifdef LOGGER_VERBOSE
    const char* msg = e.what();
    std::cerr << "Runtime Exception occured when logging:\n";
    std::cerr << msg << '\n';
#endif
    return -1;
  }
}

int lg_warn(const char* msg) {
  try {
    return Logger::instance().logWarning(msg);
  } catch(const std::exception& e) {
#ifdef LOGGER_VERBOSE
    const char* msg = e.what();
    std::cerr << "Runtime Exception occured when logging:\n";
    std::cerr << msg << '\n';
#endif
    return -1;
  }
}

int lg_destruct(void) {
  try {
    return Logger::instance().destruct();
  } catch(const std::exception& e) {
#ifdef LOGGER_VERBOSE
    const char* msg = e.what();
    std::cerr << "Runtime Exception occured when destructing the Logger:\n";
    std::cerr << msg << '\n';
#endif
    return -1;
  }
}


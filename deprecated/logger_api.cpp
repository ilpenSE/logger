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
    // if LOGGER_VERBOSE_LIB defined, print exception message to std::cerr
    // you can define it in compile options by using
    // -DLOGGER_VERBOSE_LIB or target_compile_definitions(logger PRIVATE LOGGER_VERBOSE_LIB)
#ifdef LOGGER_VERBOSE_LIB
    const char* msg = e.what();
    std::cerr << "Runtime Exception occured when initializing the Logger:\n";
    std::cerr << msg << '\n';
#endif
    return -1;
  }
}

int lg_log(const char* msg, const char* level) {
	try {
    return Logger::instance().log(msg, level);
  } catch(const std::exception& e) {
#ifdef LOGGER_VERBOSE_LIB
    const char* msg = e.what();
    std::cerr << "Runtime Exception occured when logging:\n";
    std::cerr << msg << '\n';
#endif
    return -1;
  }
}

int lg_info(const char* msg) {
	return lg_log(msg, "INFO");
}

int lg_error(const char* msg) {
	return lg_log(msg, "ERROR");
}

int lg_warn(const char* msg) {
	return lg_log(msg, "WARNING");
}

int lg_destruct(void) {
  try {
    return Logger::instance().destruct();
  } catch(const std::exception& e) {
#ifdef LOGGER_VERBOSE_LIB
    const char* msg = e.what();
    std::cerr << "Runtime Exception occured when destructing the Logger:\n";
    std::cerr << msg << '\n';
#endif
    return -1;
  }
}


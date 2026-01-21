#ifndef LOGGER_HPP
#define LOGGER_HPP

/**
	 THIS IS STB-STYLE LIBRARY HEADER OF LOGGER. SINCE THIS LOGGER IS WRITTEN IN C++,
	 YOU MUST USE C++ IN YOUR PROJECT TO USE STB STYLE HEADERS. IF YOU'RE USING
	 C, YOU HAVE TO LINK SO/DLL FILE DYNAMICALLY.

	 USAGE OF STB STYLE HEADER:
	 #define LOGGER_VERBOSE_LIB -> enables verbosed on logger runtime errors (c++ exclusive)
	 #define LOGGER_IMPLEMENTATION -> this macro enables the implementation of logger. (c++ exclusive)
	 #define LOGGER_STRIP_PREFIXES -> completely removes "lg_" prefix
	 #define LOGGER_MINIFY_PREFIXES -> minifies "lg_" into "l", like linfo instead of lg_info
	 #include "logger.h"

	 int main() {
	   lg_init();
		 lg_info("Hello, World!");
		 info("Hello, World!");
		 linfo("Hello, World!");
		 lg_destruct();
	 }

	 AND IF YOU'RE USING IMPLEMENTATION MACRO, YOU DONT HAVE TO DYANAMIC LINK THE LIBRARY
	 BUT YOU HAVE TO USE C TYPES AND FUNCTIONS AGAIN.
 */

#ifdef __cplusplus
extern "C" {
#endif

	static int lg_init(const char* logs_dir, int use_local_time);
	static int lg_destruct(void);

	static int lg_log(const char* msg, const char* level);
	static int lg_info(const char* msg);
	static int lg_warn(const char* msg);
	static int lg_error(const char* msg);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus // ONLY USE IMPLEMENTATION MACRO IN C++
// stb style implementation macros
#ifdef LOGGER_IMPLEMENTATION

// c++ includes
#include <string>

#include <stdexcept>
#include <filesystem>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include <mutex>

#include <ctime>
#include <chrono>

// LOGGER.HPP AND LOGGER.CPP MAIN CODE
#ifndef LOGGER_CLASS_HPP
#define LOGGER_CLASS_HPP

/*
  PURE C++ LOGGER MEYERS SINGLETON
  Supports thread-safe logging
  Log messages like this: 2026.01.03-15.41.46.171 [INFO] Message here
  Log files like this: 2026.01.03-15.41.46.171.log

  Log functions prints logs into standart output (std::cout) and m_logFile (std::ofstream)
   - no matter error, warning and info

  Configs:
  isLocalTime: if true, uses your local time for timestamp
  shouldThrowError: if true, throws errors on logger class exceptions (not your logError calls)

  When destruct() method is called, it closes file and set s_alive to be false. And if s_alive = false, you just cannot use log functions.

  To use this logger, you firstly have to include logger.h (externed to C) and call lg_init (which calls Logger::instance().initialize())
  And then, you can log some infos, errors and warnings.
  At the end, use destruct() to safely destruct the Logger.
  Since it has destruct(), you can always manually manage it in your destructing logic in your app or something.
*/

namespace fs = std::filesystem;

class Logger {
public:
  // meyers singleton
  static Logger& instance() {
    static Logger _ins;
    return _ins;
  }
	
  static bool isAlive() { return s_alive; }

	// set alive false and close the file
  // DO NOT use destructor, instead use seperate func for destructing
  bool destruct() noexcept {
		std::lock_guard<std::mutex> lock(mtx); // LOCK THREAD

    // file already closed
    // if file is open but alive is false, logger is in weird state and then we normalize it
    if (!m_logFile.is_open()) {
      s_alive = false;
      return true;
    }

    m_logFile.flush();
    m_logFile.close();

    if (m_logFile.is_open())
      return false;

    s_alive = false;
    return true;
	}

  bool initialize(const std::string& _logsDir, bool isLocalTime) {
		std::lock_guard<std::mutex> lock(mtx); // LOCK THREAD
		                                       // to prevent race-conditions
    // set configs
    m_isLocalTime = isLocalTime;

		// defining file name and trailing slash fix
    std::string logsDir = _logsDir;
    std::string fileName = getTime() + ".log";
    if (!_logsDir.empty() && _logsDir.back() != '/' && _logsDir.back() != '\\')
      logsDir += '/';
	  
    // check if log dir exists and a directory
    bool dirExists = fs::exists(logsDir);
    if (dirExists && !fs::is_directory(logsDir)) {
      lgierror("Given path is NOT a directory");
      return false;
    }
	  
    // tryna create if logs folder doesnt exist
    if (!dirExists) {
      if (!fs::create_directories(logsDir)) {
        lgierror("Logs folder cannot be created!");
        return false;
      }
    }
    
    std::string filePath = logsDir + fileName;
    m_logFile.open(filePath.c_str(), std::ios::out | std::ios::app);
	  
    // check defensively
    if (!m_logFile.is_open()) {
      lgierror("Log file could not be opened: " + filePath);
      return false;
    }
	  
    return true;
  }

  // log funcs
  bool logInfo(const std::string& msg) {
		return logPrivate(msg, "INFO");
	}
	
  bool logError(const std::string& msg) {
		return logPrivate(msg, "ERROR");
	}

	bool logWarning(const std::string& msg) {
		return logPrivate(msg, "WARN");
	}
	
	bool log(const std::string& msg, const std::string& level) {
		return logPrivate(msg, level);
	}
private:
	// main logging func
	bool logPrivate(const std::string& msg, const std::string& level) {
    std::lock_guard<std::mutex> lock(mtx); // LOCK THREAD
    if (!s_alive) {
      lgierror("Logger destructed!");
      return false;
    }

    std::string message = getTime() + " [" + level + "] " + msg;
	  lgiprint(message);
    m_logFile << message << '\n';

    if (!m_logFile.good()) {
      lgierror("Logging failed!");
      return false;
    }
    return true;
	}

	// timestamp wrapper - platform independent
  // milliseconds sensitivity
  std::string getTime() {
    using namespace std::chrono;

    auto now = system_clock::now(); // ms-level
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::time_t t = system_clock::to_time_t(now); // reduce to seconds
    std::tm tm{};

#if defined(_WIN32)
    if (m_isLocalTime) {
        localtime_s(&tm, &t);
    } else {
        gmtime_s(&tm, &t);
    }
#else
    if (m_isLocalTime) {
        localtime_r(&t, &tm);
    } else {
        gmtime_r(&t, &tm);
    }
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y.%m.%d-%H.%M.%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return oss.str();
	}

	// just kindly use c++17 and higher
  // logger vars and configs
	inline static bool s_alive = false;
  bool m_isLocalTime = false;
  std::ofstream m_logFile;

  std::mutex mtx;

  // singleton configs
  explicit Logger() { s_alive = true; };
  ~Logger() {};
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  void lgiprint(const std::string& msg) {
    std::cout << msg << '\n';
  }

  void lgierror(const std::string& msg) {
    std::cerr << "LOGGER RUNTIME ERROR: " << msg << '\n';
  }
};

#endif // LOGGER_CLASS_HPP

static int lg_init(const char* logs_dir, int use_local_time) {
	try {
		return Logger::instance().initialize(
			logs_dir ? logs_dir : "logs",
			use_local_time != 0
			);
	} catch (const std::exception& e) {
    // swallow logger's runtime exceptions
    return -1;
	}
	return 1;
}

static int  lg_destruct(void) {
	try {
    return Logger::instance().destruct();
  } catch(const std::exception& e) { return -1; }
}

static int lg_log(const char* msg, const char* level) {
	try {
    return Logger::instance().log(msg, level);
  } catch(const std::exception& e) { return -1; }
}

static int lg_info(const char* msg) {
	return lg_log(msg, "INFO");
}

static int lg_error(const char* msg) {
	return lg_log(msg, "ERROR");
}

static int lg_warn(const char* msg) {
	return lg_log(msg, "WARNING");
}

#ifndef LOGGER_PREFIX_MODIFIERS_GUARD
#define LOGGER_PREFIX_MODIFIERS_GUARD

// strips ONLY log functions
#ifdef LOGGER_STRIP_PREFIXES
#define info  lg_info
#define warn  lg_warn
#define error lg_error
#endif // LOGGER_STRIP_PREFIXES

// minify prefix from lg_ to l
#ifdef LOGGER_MINIFY_PREFIXES
#define linfo  lg_info
#define lwarn  lg_warn
#define lerror lg_error
#endif // LOGGER_MINIFY_PREFIXES

// do not define llog multiple times
// btw lg_log's stripped/minified version should be "llog" instead of "log"
// bcs math.h collides with this
#if defined(LOGGER_MINIFY_PREFIXES) || defined(LOGGER_STRIP_PREFIXES)
#define llog lg_log
#endif // LOGGER_STRIP_PREFIXES OR LOGGER_MINIFY_PREFIXES

#endif // LOGGER_PREFIX_MODIFIERS_GUARD

#endif // LOGGER_IMPLEMENTATION
#endif // __cplusplus

#endif // LOGGER_HPP

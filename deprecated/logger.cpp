#include "logger.hpp"
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <ctime>
#include <chrono>

namespace fs = std::filesystem;

bool Logger::s_alive = false;

// initialization, opening file
bool Logger::initialize(const std::string& logsDir,
    bool isLocalTime, bool shouldThrowError) {
  std::lock_guard<std::mutex> lock(mtx); // LOCK THREAD
                                         // to prevent race-conditions
  // set configs
  m_shouldThrowError = shouldThrowError;
  m_isLocalTime = isLocalTime;

  // check if log dir exists and a directory
  bool dirExists = fs::exists(logsDir);
  if (dirExists && !fs::is_directory(logsDir)) {
    lgerror("Given path is NOT a directory");
    return false;
  }

  // tryna create if logs folder doesnt exist
  if (!dirExists) {
    if (!fs::create_directories(logsDir)) {
      lgerror("Logs folder cannot be created!");
      return false;
    }
  }

  // defining file name and trailing slash fix
  std::string fixedLogsDir = logsDir;
  std::string fileName = getTime() + ".log";
  if (!logsDir.empty() && logsDir.back() != '/' && logsDir.back() != '\\')
    fixedLogsDir += '/';
  
  std::string filePath = fixedLogsDir + fileName;
  m_logFile.open(filePath.c_str(), std::ios::out | std::ios::app);

  // check defensively
  if (!m_logFile.is_open()) {
    lgerror("Log file could not be opened: " + filePath);
    return false;
  }

  return true;
}

// main logging func
bool Logger::logPrivate(const std::string& msg, const std::string& level) {
  std::lock_guard<std::mutex> lock(mtx); // LOCK THREAD
  if (!s_alive) {
    lgerror("Logger destructed!");
    return false;
  }

  std::string message = getTime() + " [" + level + "] " + msg;
  lgprint(message);
  m_logFile << message << '\n';

  if (!m_logFile.good()) {
    lgerror("Logging failed!");
    return false;
  }
  return true;
}

// main log func's wrappers
bool Logger::log(const std::string& msg, const std::string& level) {
  return logPrivate(msg, level);
}

bool Logger::logInfo(const std::string& msg) {
  return logPrivate(msg, "INFO");
}

bool Logger::logError(const std::string& msg) {
  return logPrivate(msg, "ERROR");
}

bool Logger::logWarning(const std::string& msg) {
  return logPrivate(msg, "WARNING");
}

// timestamp wrapper - platform independent
// milliseconds sensitivity
std::string Logger::getTime() {
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

// set alive false and close the file
// DO NOT use destructor, instead use seperate func for destructing
bool Logger::destruct() noexcept {
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

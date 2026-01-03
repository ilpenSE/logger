#ifndef LOGGER_HPP
#define LOGGER_HPP

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

#include <fstream>
#include <string>
#include <mutex>
#include <stdexcept>
#include <iostream>

class Logger {
public:
  // meyers singleton
  static Logger& instance() {
    static Logger _ins;
    return _ins;
  }
  bool isAlive() { return s_alive; }
  bool destruct() noexcept;

  bool initialize(const std::string& logsDir, bool isLocalTime, bool shouldThrowError);

  // log funcs
  bool logInfo(const std::string& msg);
  bool logError(const std::string& msg);
  bool logWarning(const std::string& msg);
  bool log(const std::string& msg, const std::string& level);
private:
  bool logPrivate(const std::string& msg, const std::string& level);
  std::string getTime();

  // logger vars and configs
  bool s_alive = false;
  bool m_isLocalTime = false;
  bool m_shouldThrowError = true;
  std::ofstream m_logFile;

  std::mutex mtx;

  // singleton configs
  explicit Logger();
  ~Logger();
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  void lgprint(const std::string& msg) {
    std::cout << msg << '\n';
  }

  void lgerror(const std::string& msg) {
    if (m_shouldThrowError) throw std::runtime_error(msg);
    else std::cerr << "ERROR: " << msg << '\n';
  }
};

#endif // LOGGER_HPP

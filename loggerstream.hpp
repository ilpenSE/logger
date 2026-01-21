#ifndef LOGGERSTREAM_HPP
#define LOGGERSTREAM_HPP

#include <sstream>
#include <string>
#include "logger.h"

class LoggerStream {
 public:
  explicit LoggerStream(std::string level)
      : m_level(level) {}

  ~LoggerStream() {
    const std::string s = m_buffer.str();
    if (!s.empty()) {
			lg_log(m_level.c_str(), s.c_str());
    }
  }

  template <typename T>
  LoggerStream& operator<<(const T& value) {
    m_buffer << value << m_delimiter;
    return *this;
  }

    // support for QStrings
# ifdef QT_CORE_LIB
# include <QString>
  LoggerStream& operator<<(const QString& value) {
    m_buffer << value.toStdString() << m_delimiter;
    return *this;
  }
# endif
 private:
  std::ostringstream m_buffer;
  const char m_delimiter = ' ';
	std::string m_level;
};

#define sinfo LoggerStream("INFO")
#define serr  LoggerStream("ERROR")
#define swarn LoggerStream("WARNING")
#define scustom LoggerStream("CUSTOM")

#endif

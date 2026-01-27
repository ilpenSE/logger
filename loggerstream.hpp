#ifndef LOGGERSTREAM_HPP
#define LOGGERSTREAM_HPP

#include <sstream>
#include <string>
#include <utility>
#include "logger.h"

class LoggerStream {
 public:
  explicit LoggerStream(std::string level)
    : m_level(std::move(level)) {}

  ~LoggerStream() {
    const std::string s = m_buffer.str();
    if (!s.empty()) {
      lg_log(m_level.c_str(), s.c_str());
    }
  }

  template <typename T>
  LoggerStream& operator<<(const T& value) {
    if (!m_first) m_buffer << m_delimiter;
    m_buffer << value;
    m_first = false;
    return *this;
  }

  // support for QStrings
# ifdef QT_CORE_LIB
# include <QString>
  LoggerStream& operator<<(const QString& value) {
    if (!m_first) m_buffer << m_delimiter;
    m_buffer << value.toUtf8().constData();
    m_first = false;
    return *this;
  }
# endif
 private:
  bool m_first = true;
  std::ostringstream m_buffer;
  const char m_delimiter = ' ';
  std::string m_level;
};

#define sinfo LoggerStream("INFO")
#define serr  LoggerStream("ERROR")
#define swarn LoggerStream("WARNING")
#define scustom LoggerStream("CUSTOM")

#endif

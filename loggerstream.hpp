/*
  The MIT License
  Copyright (c) 2026, ilpeN

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

  TLDR:
    do whatever you want, just keep the license text
*/

#ifndef LOGGERSTREAM_HPP
#define LOGGERSTREAM_HPP

#include <sstream>
#include <string>
#include <utility>
#include "logger.h"

class LoggerStream {
public:
  explicit LoggerStream(LgLogLevel level)
    : m_level(std::move(level)) {}

  ~LoggerStream() {
    const std::string s = m_buffer.str();
    if (!s.empty()) {
      lg_flog(m_level, s.c_str());
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
  LgLogLevel m_level;
};

#define sinfo   LoggerStream(LG_INFO)
#define serr    LoggerStream(LG_ERROR)
#define swarn   LoggerStream(LG_WARNING)
#define scustom LoggerStream(LG_CUSTOM)

#endif

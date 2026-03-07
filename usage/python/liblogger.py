"""
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

  Logger library importings and stuffs in Python
  Import this in your python project
"""

__all__ = ["Logger"]

from cffi import FFI
from enum import IntEnum

ffi = FFI()

ffi.cdef("""
typedef enum {
  LG_INFO = 1 << 0,
  LG_ERROR = 1 << 1,
  LG_WARNING = 1 << 2,
  LG_CUSTOM = 1 << 3,
} LgLogLevel;

typedef enum {
  LG_DROP = 1 << 0,
  LG_BLOCK = 1 << 1,
  LG_PRIORITY_BASED = 1 << 2,
} LgLogPolicy;

typedef struct Logger Logger;
typedef struct LgString LgString;
typedef struct LgMsgPack LgMsgPack;

struct LgString {
  char* data;
  size_t cap;
  size_t len;
};

struct LgMsgPack {
  LgString file_str;
  LgString stdout_str;
};

typedef int (*log_formatter_t)(const int isLocalTime, const LgLogLevel level,
                               const char* msg, LgMsgPack* pack);

typedef struct {
  int localTime;
  int printStdout;
  int maxFiles;
  LgLogPolicy logPolicy;
  log_formatter_t logFormatter;
} LoggerConfig;

Logger* lg_get_active_instance();
int lg_set_active_instance(Logger* inst);

int lg_init(Logger* instance, const char* logs_dir, LoggerConfig config);
int lg_destroy(Logger* instance);
int lg_is_alive(const Logger* instance);

int lg_finfo(const char* msg);
int lg_ferror(const char* msg);
int lg_fwarn(const char* msg);
int lg_flog(LgLogLevel level, const char* msg);

int lg_flogi(Logger* lg, LgLogLevel level, const char* msg);
int lg_finfoi(Logger* lg, const char* msg);
int lg_ferrori(Logger* lg, const char* msg);
int lg_fwarni(Logger* lg, const char* msg);

void lg_str_write_into(LgString* s, const char* str);
const char* lg_lvl_to_str(const LgLogLevel level);
int lg_get_time_str(char* buf, int isLocalTime);

Logger* lg_alloc();
void    lg_free(Logger* inst);
""")

import os
import sys

if sys.platform == "win32":
  lib_name = "logger.dll"
elif sys.platform == "darwin":
  lib_name = "liblogger.dylib"
else:
  lib_name = "liblogger.so"

dll_path = os.path.abspath(f"../../build/{lib_name}")
_logger = ffi.dlopen(dll_path)

def _decode_cstr(cstr) -> str:
  return ffi.string(cstr).decode("utf-8")

class LogLevel(IntEnum):
  INFO    = 1 << 0
  ERROR   = 1 << 1
  WARNING = 1 << 2
  CUSTOM  = 1 << 3

class LogPolicy(IntEnum):
  DROP         = 1 << 0
  SMASH_OLDEST = 1 << 1

class Logger:
  def __init__(self, _ptr=None) -> None:
    self._ptr = _ptr if _ptr is not None else _logger.lg_alloc()

  def init(self, logs_dir: str, **kwargs) -> bool:
    cfg = ffi.new("LoggerConfig*")
    cfg.printStdout  = kwargs.get("printStdout", 0)
    cfg.localTime    = kwargs.get("localTime", 0)
    cfg.maxFiles     = kwargs.get("maxFiles", 0)
    cfg.logPolicy    = kwargs.get("logPolicy", LogPolicy.DROP)
    cfg.logFormatter = kwargs.get("logFormatter", ffi.NULL)
    return bool(_logger.lg_init(self._ptr, logs_dir.encode(), cfg[0]))

  def destroy(self) -> bool:
    return bool(_logger.lg_destroy(self._ptr))

  def is_alive(self) -> bool:
    return bool(_logger.lg_is_alive(self._ptr))

  def free(self) -> None:
    _logger.lg_free(self._ptr)

  def __enter__(self):
    return self

  def __exit__(self, exc_type, exc_val, exc_tb):
    self.destroy()
    _logger.lg_free(self._ptr)
    self._ptr = None
    return False

  def __del__(self):
    if self._ptr is not None:
      _logger.lg_free(self._ptr)
      self._ptr = None

  # Log Function Transpilations
  def info(self, msg: str) -> bool:
    return bool(_logger.lg_finfo(msg.encode()))

  def error(self, msg: str) -> bool:
    return bool(_logger.lg_ferror(msg.encode()))

  def warn(self, msg: str) -> bool:
    return bool(_logger.lg_fwarn(msg.encode()))

  def log(self, level: LogLevel, msg: str) -> bool:
    return bool(_logger.lg_flog(level, msg.encode()))

  # Explicit instances
  def infoi(self, msg: str) -> bool:
    return bool(_logger.lg_finfoi(self._ptr, msg.encode()))

  def errori(self, msg: str) -> bool:
    return bool(_logger.lg_ferrori(self._ptr, msg.encode()))

  def warni(self, msg: str) -> bool:
    return bool(_logger.lg_fwarni(self._ptr, msg.encode()))

  def logi(self, level: LogLevel, msg: str) -> bool:
    return bool(_logger.lg_flogi(self._ptr, level, msg.encode()))
  
  @staticmethod
  def get_active_instance() -> "Logger":
    return Logger(_ptr=_logger.lg_get_active_instance())

  @staticmethod
  def set_active_instance(inst: "Logger") -> bool:
    return bool(_logger.lg_set_active_instance(inst._ptr))

  @staticmethod
  def get_time_str(local_time: bool) -> str:
    buf = ffi.new("char[24]")
    if not _logger.lg_get_time_str(buf, local_time):
      return ""
    return _decode_cstr(buf)

  @staticmethod
  def lvl_to_str(level: LogLevel) -> str:
    return _decode_cstr(_logger.lg_lvl_to_str(level))

  # Decorator for custom formatter
  # Func is full python function (parameters etc.) go to main.py
  @staticmethod
  def logFormatter(func):
    @ffi.callback("int(const int, LgLogLevel, const char*, LgMsgPack*)")
    def wrapper(local_time, level, msg, pack):
      p_level_str = _decode_cstr(_logger.lg_lvl_to_str(level))
      p_local_time = True if local_time == 1 else False
      p_msg_str = _decode_cstr(msg)
  
      # Get stdout string
      if pack.stdout_str.data != ffi.NULL:
        result: str = func(p_local_time, p_level_str, p_msg_str, True) + "\n"
        _logger.lg_str_write_into(ffi.addressof(pack.stdout_str), result.encode())
  
      # Get file string
      if pack.file_str.data != ffi.NULL:
        result: str = func(p_local_time, p_level_str, p_msg_str, False) + "\n"
        _logger.lg_str_write_into(ffi.addressof(pack.file_str), result.encode())
  
      return 1
  
    return wrapper

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

__all__ = ["Logger", "LogOutType", "LogPolicy", "LogLevel"]

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

typedef enum {
  LG_OUT_TTY = 0,
  LG_OUT_FILE = 1,
  LG_OUT_NET = 2,
  // Add more out types here
  // Dont forget to update LOGGER_MAX_OUT_TYPES
} LgOutType;

#define LOGGER_MAX_OUT_TYPES 3
#define LOGGER_MAX_MSG_SIZE 256
#define LOGGER_MAX_SINKS 8

typedef struct Logger Logger;

typedef struct LgString {
  char data[LOGGER_MAX_MSG_SIZE];
  size_t len;
} LgString;

typedef LgString LgMsgPack[LOGGER_MAX_OUT_TYPES];

typedef struct {
  FILE* file;
  LgOutType type;
} LgSink;

typedef struct {
  LgSink items[LOGGER_MAX_SINKS];
  size_t count;
} LgSinks;

typedef int (*log_formatter_t)(
  const char* time_str,
  LgLogLevel level,
  const char* msg,
  uint32_t needed,
  LgMsgPack out
);

typedef struct {
  int localTime;
  int maxFiles;
  int generateDefaultFile;
  LgSinks sinks;
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
LoggerConfig lg_get_defaults();
int lg_append_sink(LoggerConfig* config, FILE* f, LgOutType type);

FILE* lg_get_stdout();
FILE* lg_get_stderr();
FILE* lg_fopen(const char* name);

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

def _contains_flag(needed, flag) -> bool:
  return bool(needed & (1 << flag))

def _decode_cstr(cstr) -> str:
  return ffi.string(cstr).decode("utf-8")

class LogLevel(IntEnum):
  INFO    = 1 << 0
  ERROR   = 1 << 1
  WARNING = 1 << 2
  CUSTOM  = 1 << 3

  def __str__(self):
    return self.name

class LogPolicy(IntEnum):
  DROP           = 1 << 0
  BLOCK          = 1 << 1
  PRIORITY_BASED = 1 << 2

class LogOutType(IntEnum):
  TTY  = 0
  FILE = 1
  NET  = 2

class LoggerConfig:
  _FIELDS = {
    "localTime":           lambda v: 1 if v else 0,
    "generateDefaultFile": lambda v: 1 if v else 0,
    "maxFiles":            lambda v: int(v),
    "logPolicy":           lambda v: int(v),
    "logFormatter":        lambda v: ffi.NULL if v is None else v,
  }

  def __init__(self, **kwargs):
    object.__setattr__(self, "_c", ffi.new("LoggerConfig*"))
    for key, val in kwargs.items():
      setattr(self, key, val)

  def __setattr__(self, name, value):
    if name in self._FIELDS:
      setattr(self._c, name, self._FIELDS[name](value))
    else:
      object.__setattr__(self, name, value)

  def __getattr__(self, name):
    if name in self._FIELDS:
      return getattr(self._c, name)
    raise AttributeError(f"No field: {name}")

  def append_sink(self, file_ptr, out_type: LogOutType) -> bool:
    return bool(_logger.lg_append_sink(self._c, file_ptr, out_type))

  def get_c_struct(self):
    return self._c[0]

  @staticmethod
  def default() -> "LoggerConfig":
    c_defaults = _logger.lg_get_defaults()
    cfg = LoggerConfig()
    cfg._c[0] = c_defaults
    return cfg

class Logger:
  def __init__(self, _ptr=None) -> None:
    self._ptr = _ptr if _ptr is not None else _logger.lg_alloc()

  def init(self, logs_dir: str, config: "LoggerConfig") -> bool:
    return bool(_logger.lg_init(self._ptr, logs_dir.encode(), config.get_c_struct()))

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

  # Decorator for custom formatter
  # Func is full python function (parameters etc.) go to main.py
  # Func signature: func(str, str, str, LogOutType): str
  @staticmethod
  def logFormatter(func):
    @ffi.callback("int(const char*, LgLogLevel, const char*, uint32_t, LgString*)")
    def wrapper(time_str, level, msg, needed, pack):
      p_level_str = _decode_cstr(_logger.lg_lvl_to_str(level))
      p_time_str  = _decode_cstr(time_str)
      p_msg_str   = _decode_cstr(msg)

      tty_str  = ffi.addressof(pack, LogOutType.TTY)
      file_str = ffi.addressof(pack, LogOutType.FILE)
      net_str  = ffi.addressof(pack, LogOutType.NET)

      # TTY Output Type
      if _contains_flag(needed, LogOutType.TTY):
        result_tty = func(p_time_str, p_level_str, p_msg_str, LogOutType.TTY) + "\n"
        _logger.lg_str_write_into(tty_str, result_tty.encode())

      # File Output Type
      if _contains_flag(needed, LogOutType.FILE):
        result_file = func(p_time_str, p_level_str, p_msg_str, LogOutType.FILE) + "\n"
        _logger.lg_str_write_into(file_str, result_file.encode())

      # Net Output Type
      if _contains_flag(needed, LogOutType.NET):
        result_net = func(p_time_str, p_level_str, p_msg_str, LogOutType.NET) + "\n"
        _logger.lg_str_write_into(net_str, result_net.encode())
      return 1
    return wrapper

class LoggerUtils:
  @staticmethod
  def get_instance() -> "Logger | None":
    inst = _logger.lg_get_active_instance()
    return None if inst == ffi.NULL else Logger(inst)

  @staticmethod
  def set_instance(lg: "Logger") -> bool:
    return bool(_logger.lg_set_active_instance(lg._ptr))

  @staticmethod
  def get_time_str(local_time: bool) -> str:
    buf = ffi.new("char[24]")
    if not _logger.lg_get_time_str(buf, local_time):
      return ""
    return _decode_cstr(buf)

  @staticmethod
  def file_from_path(path: str):
    return _logger.lg_fopen(path.encode())

  @staticmethod
  def stdout():
    return _logger.lg_get_stdout()

  @staticmethod
  def stderr():
    return _logger.lg_get_stderr()

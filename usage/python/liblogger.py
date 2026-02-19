"""
  Logger library importings and stuffs in Python
  Import this in your python project
"""

from cffi import FFI

ffi = FFI()

ffi.cdef("""
typedef enum {
  LG_INFO = 1 << 0,
  LG_ERROR = 1 << 1,
  LG_WARNING = 1 << 2,
  LG_CUSTOM = 1 << 3,
} lg_log_level;

typedef struct {
  char* data;
  size_t cap;
  size_t len;
} string;

typedef struct {
  string stdout_str;
  string file_str;
} lg_msg_pack;

typedef void (*log_formatter_t)(const char* time_str, const lg_log_level level,
  const char* msg, lg_msg_pack* pack);

typedef struct {
  int localTime;
  int printStdout;
  log_formatter_t logFormatter;
} LoggerConfig;

typedef struct Logger Logger;

Logger* lg_get_active_instance();
int lg_set_active_instance(Logger* inst);

int lg_init(Logger* instance, const char* logs_dir, LoggerConfig config);
int lg_destroy(Logger* instance);
int lg_is_alive(const Logger* instance);

int lg_finfo(const char* msg);
int lg_ferror(const char* msg);
int lg_fwarn(const char* msg);
int lg_flog(lg_log_level level, const char* msg);

Logger* lg_alloc();
void    lg_free(Logger* inst);
""")

logger = ffi.dlopen("../../build/liblogger.so")

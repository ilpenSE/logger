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

typedef enum {
  LG_DROP = 1 << 0,
  LG_BLOCK = 1 << 1,
  LG_SMASH_OLDEST = 1 << 2,
  LG_PRIORITY_BASED = 1 << 3,
} lg_log_policy;

typedef struct Logger Logger;
typedef struct lg_string lg_string;
typedef struct lg_msg_pack lg_msg_pack;

struct lg_string {
  char* data;
  size_t cap;
  size_t len;
};

struct lg_msg_pack {
  lg_string file_str;
  lg_string stdout_str;
};

typedef void (*log_formatter_t)(const char* time_str, const lg_log_level level,
  const char* msg, lg_msg_pack* pack);

typedef struct {
  int localTime;
  int printStdout;
  lg_log_policy policy;
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
int lg_flog(lg_log_level level, const char* msg);

int lg_flogi(Logger* lg, lg_log_level level, const char* msg);
int lg_finfoi(Logger* lg, const char* msg);
int lg_ferrori(Logger* lg, const char* msg);
int lg_fwarni(Logger* lg, const char* msg);

void lg_str_write_into(lg_string* s, const char* str);
const char* lg_lvl_to_str(const lg_log_level level);

Logger* lg_alloc();
void    lg_free(Logger* inst);
""")

logger = ffi.dlopen("../../build/liblogger.so")

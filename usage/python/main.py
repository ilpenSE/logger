from cffi import FFI
import time

def main():
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

int lg_init(const char* logs_dir, LoggerConfig config);
int lg_log_s(const lg_log_level level, const char* msg);
int lg_info_s(const char* msg);
int lg_warn_s(const char* msg);
int lg_error_s(const char* msg);
int lg_destruct(void);
""")
  lib = ffi.dlopen("../../build/liblogger.so")

  cfg = ffi.new("LoggerConfig *")
  cfg.printStdout = 1
  cfg.localTime = 1
  cfg.logFormatter = ffi.NULL

  if not lib.lg_init(b"logs", cfg[0]):
    return

  lib.lg_info_s(b"Hello from Python!")
  lib.lg_error_s(b"Error from Python!")
  lib.lg_warn_s(b"Warning %10 from Python!")

  #time.sleep(1)
  if not lib.lg_destruct():
    print("[PYTHON] DESTRUCT FAIL")
    return

if __name__ == "__main__":
  main()

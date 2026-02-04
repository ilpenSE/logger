from cffi import FFI
import time

def main():
  ffi = FFI()

  ffi.cdef("""
typedef enum {
  LG_FATAL_ERROR = -1,
  LG_RUNTIME_ERROR = 0,
  LG_SUCCESS = 1,
  LG_NOT_VALID_DIR = 2,
  LG_NOT_OPEN_FILE = 3,
  LG_RING_FULL = 4
} lg_result_t;

typedef int (*log_formatter_t)(const char*, const char*, const char*, char*, size_t);

typedef struct {
  int localTime;
  int printStdout;
  log_formatter_t logFormatter;
} LoggerConfig;

lg_result_t lg_init(const char* logs_dir, LoggerConfig config);
lg_result_t lg_log_s(const char* level, const char* msg);
lg_result_t lg_info_s(const char* msg);
lg_result_t lg_warn_s(const char* msg);
lg_result_t lg_error_s(const char* msg);
lg_result_t lg_destruct(void);
""")
  lib = ffi.dlopen("../../build/liblogger.so")

  cfg = ffi.new("LoggerConfig *")
  cfg.printStdout = 1
  cfg.localTime = 1
  cfg.logFormatter = ffi.NULL

  if lib.lg_init(b"logs", cfg[0]) != lib.LG_SUCCESS:
    return

  lib.lg_info_s(b"Hello from Python!")
  lib.lg_error_s(b"Error from Python!")
  lib.lg_warn_s(b"Warning from Python!")

  #time.sleep(1)
  if lib.lg_destruct() != lib.LG_SUCCESS:
    print("[PYTHON] DESTRUCT FAIL")
    return

if __name__ == "__main__":
  main()

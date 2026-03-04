from liblogger import logger, ffi
import time, ctypes

def decode_cstr(cstr) -> str:
  return ffi.string(cstr).decode("utf-8")

@ffi.callback("int(const int, lg_log_level, const char*, lg_msg_pack*)")
def myFormatter(local_time, level, msg, pack):
  # Getting time_str
  buf = ffi.new("char[24]")
  if not logger.lg_get_time_str(buf, local_time) == 1:
    return 0

  # Create formatted string to write
  timeStr = decode_cstr(buf)
  levelStr = decode_cstr(logger.lg_lvl_to_str(level))
  msgStr = decode_cstr(msg)
  formatted = f"{timeStr} {levelStr}: {msgStr}\n".encode()

  # String that goes into file
  if pack.file_str.data != ffi.NULL:
    logger.lg_str_write_into(ffi.addressof(pack.file_str), formatted)

  # String that goes into stdout
  if pack.stdout_str.data != ffi.NULL:
    logger.lg_str_write_into(ffi.addressof(pack.stdout_str), formatted)
  return 1

def main():
  cfg = ffi.new("LoggerConfig*")
  cfg.printStdout = 1
  cfg.localTime = 1
  cfg.policy = logger.LG_DROP
  cfg.logFormatter = myFormatter # ffi.NULL

  inst = logger.lg_alloc()
  if not logger.lg_init(inst, b"logs", cfg[0]):
    return

  logger.lg_finfo(b"Hello from Python!")
  logger.lg_ferror(b"Error from Python!")
  logger.lg_fwarn(b"Warning %10 from Python!")

  #time.sleep(0.1)
  if not logger.lg_destroy(inst):
    print("[PYTHON] DESTRUCT FAIL")
    logger.lg_free(inst)
    return

  logger.lg_free(inst)

if __name__ == "__main__":
  main()

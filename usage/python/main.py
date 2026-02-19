from liblogger import logger, ffi
import time

def main():
  cfg = ffi.new("LoggerConfig*")
  cfg.printStdout = 1
  cfg.localTime = 1
  cfg.logFormatter = ffi.NULL

  inst = logger.lg_alloc();
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

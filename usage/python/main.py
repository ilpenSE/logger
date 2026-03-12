from liblogger import ffi, Logger, LogOutType, LoggerConfig, LoggerUtils
import time, ctypes

bash_aqua = "\x1b[36m"
bash_reset = "\x1b[0m"

@Logger.logFormatter
def myFormatter(time_str: str, level: str, msg: str, out_type: LogOutType) -> str:
  match out_type:
    case LogOutType.TTY:
      formatted = f"{time_str} {bash_aqua}{level}{bash_reset}: {msg}"
    case LogOutType.FILE:
      formatted = f"{time_str} {level}: {msg}"
    case LogOutType.NET:
      formatted = f'{{"timestamp":"{time_str}","level":"{level}","message":"{msg}"}}'
    case _:
      f"{time_str} {level}: {msg}"

  return formatted

def main():
  lg = Logger()
  conf = LoggerConfig()
  conf.generateDefaultFile = True
  conf.localTime = True
  conf.append_sink(LoggerUtils.stdout(), LogOutType.TTY)
  conf.append_sink(LoggerUtils.file_from_path("log.log"), LogOutType.NET)
  conf.logFormatter = None

  if not lg.init("logs", conf):
    print("[PYTHON] Logger initialization failed!")
    return

  lg.info("Hello from Python!")
  lg.error("Error from Python!")
  lg.warn("Warning %10 from Python!")

  if not lg.destroy():
    print("[PYTHON] Logger destroy failed!")
    return

  # GC will free, so we dont call free manually

if __name__ == "__main__":
  main()

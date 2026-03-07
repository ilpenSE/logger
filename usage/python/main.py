from liblogger import Logger
import time, ctypes

@Logger.logFormatter
def myFormatter(local_time: bool, level: str, msg: str, stdout: bool) -> str:
  time_str = Logger.get_time_str(local_time)

  if stdout:
    formatted = f"{time_str} {level}: {msg}"
  else:
    formatted = f"{time_str} {level}: {msg}"
  return formatted

def main():
  lg = Logger()
  if not lg.init("logs", printStdout=1, localTime=1, maxFiles=10, logFormatter=myFormatter):
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

package main

import(
  "fmt"
  "myapp/logger"
)

func main() {
  lg := logger.New()
  defer lg.Free()

  cfg := logger.LoggerConfig{
    LocalTime:           true,
    MaxFiles:            10,
    GenerateDefaultFile: true,
    Policy:              logger.PolicyDrop,
  };
  cfg.AppendSink(logger.Stderr(), logger.OutTTY);

  if err := lg.Init("logs", cfg); err != nil {
    fmt.Println("Couldn't initialize logger");
    return
  }

  lg.Info("Hello from Go!");
  lg.Warn("Warning from Go!");
  lg.Error("Error from Go!");

  if err := lg.Destroy(); err != nil {
    fmt.Println("Couldn't destroy logger");
    return
  }
}

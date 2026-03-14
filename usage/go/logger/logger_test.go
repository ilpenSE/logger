package logger

import (
  "fmt"
  "testing"
)

func TestLogger(t *testing.T) {
  lg := New()
  defer lg.Free()

  if err := lg.InitDefaults("/tmp/go_logger_test"); err != nil {
    t.Fatalf("InitDefaults: %v", err)
  }

  if err := lg.SetActive(); err != nil {
    t.Fatalf("SetActive: %v", err)
  }

  lg.Info("hello from go info")
  lg.Warn("hello from go warn")
  lg.Error("hello from go error")
  lg.Custom("hello from go custom")

  // global helpers (uses active instance)
  GlobalInfo("global info")
  GlobalWarn("global warn")
  GlobalError("global error")

  fmt.Println("LevelToStr(LevelInfo):", LevelToStr(LevelInfo))
  fmt.Println("IsAlive:", lg.IsAlive())

  if err := lg.Destroy(); err != nil {
    t.Fatalf("Destroy: %v", err)
  }
}

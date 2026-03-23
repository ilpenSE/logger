package logger

/*
#cgo CFLAGS: -I../../../build
#cgo LDFLAGS: -L../../../build -lpthread -l:liblogger.a
#include <logger.h>
#include <stddef.h>
#include <stdlib.h>
*/
import "C"
import (
  "errors"
  "unsafe"
)

// LogLevel maps to LgLogLevel
type LogLevel int
const (
  LevelInfo    LogLevel = LogLevel(C.LG_INFO)
  LevelError   LogLevel = LogLevel(C.LG_ERROR)
  LevelWarning LogLevel = LogLevel(C.LG_WARNING)
  LevelCustom  LogLevel = LogLevel(C.LG_CUSTOM)
)

// LogPolicy maps to LgLogPolicy
type LogPolicy int
const (
  PolicyDrop          LogPolicy = LogPolicy(C.LG_DROP)
  PolicyBlock         LogPolicy = LogPolicy(C.LG_BLOCK)
  PolicyPriorityBased LogPolicy = LogPolicy(C.LG_PRIORITY_BASED)
)

// OutType maps to LgOutType
type OutType int
const (
  OutTTY  OutType = OutType(C.LG_OUT_TTY)
  OutFile OutType = OutType(C.LG_OUT_FILE)
  OutNet  OutType = OutType(C.LG_OUT_NET)
)

// Logger wraps a C Logger instance
type Logger struct {
  ptr *C.Logger
}

type Sink struct {
  File    unsafe.Pointer // C.FILE*
  OutType OutType
}

// LoggerConfig wraps a C LoggerConfig struct
type LoggerConfig struct {
  LocalTime           bool
  MaxFiles            int
  GenerateDefaultFile bool
  Policy              LogPolicy
  Sinks               []Sink
}

func (c *LoggerConfig) AppendSink(file unsafe.Pointer, outType OutType) {
  c.Sinks = append(c.Sinks, Sink{File: file, OutType: outType})
}

func (c LoggerConfig) toC() C.LoggerConfig {
  var cfg C.LoggerConfig
  cfg.localTime           = boolToC(c.LocalTime)
  cfg.maxFiles            = C.int(c.MaxFiles)
  cfg.generateDefaultFile = boolToC(c.GenerateDefaultFile)
  cfg.logPolicy           = C.LgLogPolicy(c.Policy)

  for _, s := range c.Sinks {
    C.lg_append_sink(&cfg, (*C.FILE)(s.File), C.LgOutType(s.OutType))
  }
  return cfg
}

// New allocates and returns a new Logger instance (heap allocated via lg_alloc)
func New() *Logger {
  return &Logger{ptr: C.lg_alloc()}
}

// Free releases the logger memory (call after Destroy)
func (l *Logger) Free() {
  if l.ptr != nil {
    C.lg_free(l.ptr)
    l.ptr = nil
  }
}

// InitDefaults initializes the logger with default config
func (l *Logger) InitDefaults(logsDir string) error {
  dir := C.CString(logsDir)
  defer C.free(unsafe.Pointer(dir))
  if C.lg_init_defaults(l.ptr, dir) == 0 {
    return errors.New("lg_init_defaults failed")
  }
  return nil
}

// Init initializes with a full config
func (l *Logger) Init(logsDir string, config LoggerConfig) error {
  dir := C.CString(logsDir)
  defer C.free(unsafe.Pointer(dir))

  if C.lg_init(l.ptr, dir, config.toC()) == 0 {
    return errors.New("lg_init failed")
  }
  return nil
}

// Destroy shuts down the logger (safe from any single thread)
func (l *Logger) Destroy() error {
  if C.lg_destroy(l.ptr) == 0 {
    return errors.New("lg_destroy failed")
  }
  return nil
}

// IsAlive returns true if the logger is running
func (l *Logger) IsAlive() bool {
  return C.lg_is_alive(l.ptr) != 0
}

// SetActive sets this logger as the active global instance
func (l *Logger) SetActive() error {
  if C.lg_set_active_instance(l.ptr) == 0 {
    return errors.New("lg_set_active_instance failed")
  }
  return nil
}

// Log logs a message at the given level to this explicit instance
func (l *Logger) Log(level LogLevel, msg string) error {
  cs := C.CString(msg)
  defer C.free(unsafe.Pointer(cs))
  if C.lg_log_(l.ptr, C.LgLogLevel(level), cs, C.size_t(len(msg))) == 0 {
    return errors.New("lg_log_ failed")
  }
  return nil
}

// Info logs at INFO level
func (l *Logger) Info(msg string) error { return l.Log(LevelInfo, msg) }

// Error logs at ERROR level
func (l *Logger) Error(msg string) error { return l.Log(LevelError, msg) }

// Warn logs at WARNING level
func (l *Logger) Warn(msg string) error { return l.Log(LevelWarning, msg) }

// Custom logs at CUSTOM level
func (l *Logger) Custom(msg string) error { return l.Log(LevelCustom, msg) }

// FLog logs a pre-formatted message at the given level to this instance
func (l *Logger) FLog(level LogLevel, msg string) error {
  cs := C.CString(msg)
  defer C.free(unsafe.Pointer(cs))
  if C.lg_flogi(l.ptr, C.LgLogLevel(level), cs) == 0 {
    return errors.New("lg_flogi failed")
  }
  return nil
}

// Global / active instance helpers

// GetActive returns the current active Logger instance (wrapped)
func GetActive() *Logger {
  return &Logger{ptr: C.lg_get_active_instance()}
}

// GlobalInfo logs INFO on the active instance
func GlobalInfo(msg string) error {
  cs := C.CString(msg)
  defer C.free(unsafe.Pointer(cs))
  if C.lg_finfo(cs) == 0 {
    return errors.New("lg_finfo failed")
  }
  return nil
}

// GlobalError logs ERROR on the active instance
func GlobalError(msg string) error {
  cs := C.CString(msg)
  defer C.free(unsafe.Pointer(cs))
  if C.lg_ferror(cs) == 0 {
    return errors.New("lg_ferror failed")
  }
  return nil
}

// GlobalWarn logs WARNING on the active instance
func GlobalWarn(msg string) error {
  cs := C.CString(msg)
  defer C.free(unsafe.Pointer(cs))
  if C.lg_fwarn(cs) == 0 {
    return errors.New("lg_fwarn failed")
  }
  return nil
}

// LevelToStr returns the string name of a log level
func LevelToStr(level LogLevel) string {
  return C.GoString(C.lg_lvl_to_str(C.LgLogLevel(level)))
}

// Stdout returns C stdout FILE* (for use with lg_append_sink)
func Stdout() unsafe.Pointer { return unsafe.Pointer(C.lg_get_stdout()) }

// Stderr returns C stderr FILE* (for use with lg_append_sink)
func Stderr() unsafe.Pointer { return unsafe.Pointer(C.lg_get_stderr()) }

// helpers

func boolToC(b bool) C.int {
  if b {
    return 1
  }
  return 0
}

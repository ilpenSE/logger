#![allow(dead_code)]
use std::os::raw::{c_char, c_int, c_void};

// Log Levels
#[repr(C)]
pub enum LgLogLevel {
  LgInfo    = 1 << 0,
  LgError   = 1 << 1,
  LgWarning = 1 << 2,
  LgCustom  = 1 << 3,
}

// Config struct
#[repr(C)]
pub struct LoggerConfig {
  pub local_time:     c_int,
  pub print_stdout:   c_int,
  pub log_formatter:  Option<unsafe extern "C" fn(
    *const c_char, *const LgLogLevel, *const c_char, *mut LgMsgPack) -> c_void>,
}

// These are forward-declared in header
// Logger instance
#[repr(C)]
pub struct Logger {
  _private: [u8; 0],
}
// Logger message pack
#[repr(C)]
pub struct LgMsgPack {
  _private: [u8; 0],
}

// Functions
#[link(name = "logger")]
unsafe extern "C" {
  // Alloc functions
  pub fn lg_alloc() -> *mut Logger;
  pub fn lg_free(inst: *mut Logger) -> c_void;
  
  // Lifetime functions
  pub fn lg_init(inst: *mut Logger, logs_dir: *const c_char, config: LoggerConfig) -> c_int;
  pub fn lg_destroy(inst: *mut Logger) -> c_int;
  
  // Log functions
  pub fn lg_finfo(msg: *const c_char) -> c_int;
  pub fn lg_ferror(msg: *const c_char) -> c_int;
  pub fn lg_fwarn(msg: *const c_char) -> c_int;
}

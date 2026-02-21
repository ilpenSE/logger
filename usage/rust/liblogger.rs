#![allow(dead_code)]
use std::os::raw::{c_char, c_int, c_void};

// Log Levels
#[repr(C)]
#[derive(Copy, Clone)]
pub enum LgLogLevel {
  Info    = 1 << 0,
  Error   = 1 << 1,
  Warning = 1 << 2,
  Custom  = 1 << 3,
}

// Log Policies
#[repr(C)]
#[derive(Copy, Clone)]
pub enum LgLogPolicy {
  Drop          = 1 << 0,
  Block         = 1 << 1,
  SmashOldest   = 1 << 2,
  PriorityBased = 1 << 3,
}

pub type LogFormatterT = unsafe extern "C" fn(
    *const c_char, LgLogLevel, *const c_char, *mut LgMsgPack);

// Config struct
#[repr(C)]
pub struct LoggerConfig {
  pub local_time:     c_int,
  pub print_stdout:   c_int,
  pub policy:         LgLogPolicy,
  pub log_formatter:  Option<LogFormatterT>,
}

// This is forward-declared in header
// Logger instance (opaque struct)
#[repr(C)]
#[derive(Copy, Clone)]
pub struct Logger {
  _private: [u8; 0],
}

// Logger lg_string struct
#[repr(C)]
#[derive(Copy, Clone)]
pub struct LgString {
  pub data: *mut c_char,
  pub cap: usize,
  pub len: usize,
}

// Logger message pack
#[repr(C)]
#[derive(Copy, Clone)]
pub struct LgMsgPack {
  pub file_str: LgString,
  pub stdout_str: LgString,
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

  // Custom formatter functions
  pub fn lg_lvl_to_str(level: LgLogLevel) -> *const c_char;
  pub fn lg_str_write_into(s: *mut LgString, str: *const c_char) -> c_void;

  // Log functions
  // Implicit instances
  pub fn lg_flog(level: LgLogLevel, msg: *const c_char) -> c_int;
  pub fn lg_finfo(msg: *const c_char) -> c_int;
  pub fn lg_ferror(msg: *const c_char) -> c_int;
  pub fn lg_fwarn(msg: *const c_char) -> c_int;

  // Explicit instances
  pub fn lg_flogi(inst: *mut Logger, level: LgLogLevel, msg: *const c_char) -> c_int;
  pub fn lg_finfoi(inst: *mut Logger, msg: *const c_char) -> c_int;
  pub fn lg_ferrori(inst: *mut Logger, msg: *const c_char) -> c_int;
  pub fn lg_fwarni(inst: *mut Logger, msg: *const c_char) -> c_int;
}

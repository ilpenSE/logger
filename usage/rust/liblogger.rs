#![allow(dead_code)]
use std::os::raw::{c_char, c_int, c_void};

pub const LOGGER_MAX_MSG_SIZE: usize = 1024;
pub const LOGGER_TIME_STR_SIZE: usize = 24;

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
  PriorityBased = 1 << 2,
}

// Message Out Types
#[repr(C)]
#[derive(Copy, Clone,PartialEq)]
pub enum LgOutType {
  TTY = 0,
  File = 1,
  Net = 2,
  Max = 3,
}

pub type LogFormatterT = unsafe extern "C" fn(
    c_int, LgLogLevel, *const c_char, u32, *mut LgString) -> c_int;

// Config struct
#[repr(C)]
pub struct LoggerConfig {
  pub local_time:     c_int,
  pub print_stdout:   c_int,
  pub log_policy:     LgLogPolicy,
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
  pub data: [c_char; LOGGER_MAX_MSG_SIZE],
  pub len: usize,
}

pub fn _contains_flag(needed: u32, flag: LgOutType) -> bool {
  needed & (1 << flag as u32) != 0
}

#[macro_export]
macro_rules! lg_formatter {
  ($name:ident, $body:expr) => {
    unsafe extern "C" fn $name(
      local_time: c_int,
      level: LgLogLevel,
      msg: *const c_char,
      needed: u32,
      pack: *mut LgString,
    ) -> c_int {
      let func: fn(&str, &str, &str, LgOutType) -> String = $body;

      let level_str = unsafe {
        CStr::from_ptr(lg_lvl_to_str(level)).to_str().unwrap_or("")
      };
      let msg_str = unsafe {
        CStr::from_ptr(msg).to_str().unwrap_or("")
      };

      let mut time_buf = [0i8; LOGGER_TIME_STR_SIZE];
      let time_str: &str = unsafe {
        if lg_get_time_str(time_buf.as_mut_ptr(), local_time) == 1 {
          CStr::from_ptr(time_buf.as_ptr()).to_str().unwrap_or("")
        } else { "" }
      };

      // TTY
      if _contains_flag(needed, LgOutType::TTY) {
        let tty_result = func(time_str, level_str, msg_str, LgOutType::TTY) + "\n";
        let tty_cstr = CString::new(tty_result).unwrap();
        unsafe { lg_str_write_into(pack.add(LgOutType::TTY as usize), tty_cstr.as_ptr()); }
      }

      // FILE
      if _contains_flag(needed, LgOutType::File) {
        let file_result = func(time_str, level_str, msg_str, LgOutType::File) + "\n";
        let file_cstr = CString::new(file_result).unwrap();
        unsafe { lg_str_write_into(pack.add(LgOutType::File as usize), file_cstr.as_ptr()); }
      }
      return 1;
    }
  };
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
  pub fn lg_get_time_str(buf: *mut c_char, isLocalTime: c_int) -> c_int;

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

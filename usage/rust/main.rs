// shut the fuck up rust
#![allow(unused_variables)]
#![allow(unused_imports)]
#![allow(dead_code)]

use std::ffi::CString;
use std::os::raw::{c_char, c_int};
use std::fmt;

// lg_result_t enum
#[repr(i32)]
#[derive(Debug, PartialEq, Copy, Clone)]
pub enum LgResult {
  FatalError = -1,
  RuntimeError = 0,
  Success = 1,
  NotValidDir = 2,
  NotOpenFile = 3,
  RingFull = 4,
}

impl fmt::Display for LgResult {
  fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
    write!(f, "{}", *self as i32)
  }
}

// logger config struct
#[repr(C)]
pub struct LoggerConfig {
  pub local_time: c_int,
  pub print_stdout: c_int,
  // Callback fonksiyonu (şu an kullanmıyoruz, NULL göndereceğiz)
  pub log_formatter: Option<unsafe extern "C" fn(*const c_char, *const c_char, *const c_char, *mut c_char, usize) -> c_int>,
}

// functions

#[link(name = "logger")]
unsafe extern "C" {
  fn lg_init(logs_dir: *const c_char, config: LoggerConfig) -> LgResult;
  fn lg_info_s(msg: *const c_char) -> LgResult;
  fn lg_error_s(msg: *const c_char) -> LgResult;
  fn lg_warn_s(msg: *const c_char) -> LgResult;
  fn lg_destruct() -> LgResult;
}

fn main() {
  let logs_dir = CString::new("logs").unwrap();

  let config = LoggerConfig {
    local_time: 1,
    print_stdout: 1,
    log_formatter: None,
  };
  
	unsafe {
    let ires = lg_init(logs_dir.as_ptr(), config);
    if ires != LgResult::Success {
      println!("Logger initialization failed with code {ires}!");
      return;
    }

    // 10k stress-test
    for i in 0..10000 {
      //println!("{i}");
      let fmt = format!("Fuck Rust {:04} times", i);
      let msg = CString::new(fmt).unwrap();
      lg_info_s(msg.as_ptr());
      //lg_error_s(msg.as_ptr());
      //lg_warn_s(msg.as_ptr());
    }
    
    let dres = lg_destruct();
    if dres != LgResult::Success {
      println!("Logger destruct failed with code {dres}!");
      return;
    }
  }
}

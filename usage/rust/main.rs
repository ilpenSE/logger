// shut the fuck up rust
#![allow(unused_variables)]
#![allow(unused_imports)]
#![allow(dead_code)]
#![allow(nonstandard_style)]

use std::ffi::CString;
use std::ffi::CStr;
use std::os::raw::{c_char, c_int, c_void};
use std::fmt;
use std::ptr::null_mut;

mod liblogger;
use liblogger::*;

// rust doing rust thing
unsafe fn cstr_to_string(cstr: *const c_char) -> String {
  unsafe { CStr::from_ptr(cstr).to_string_lossy().into_owned() }
}

unsafe extern "C" fn myFormatter(
  time_str: *const c_char,
  level: LgLogLevel,
  msg: *const c_char,
  pack: *mut LgMsgPack
) {
  unsafe {
    let formatted = CString::new(format!(
      "{} {}: {}\n",
      cstr_to_string(time_str),
      cstr_to_string(lg_lvl_to_str(level)),
      cstr_to_string(msg),
    )).unwrap();

    let file_str = (*pack).file_str;
    if file_str.data != null_mut() && file_str.cap != 0 {
      lg_str_write_into(&mut (*pack).file_str, formatted.as_ptr());
    }

    let stdout_str = (*pack).stdout_str;
    if stdout_str.data != null_mut() && stdout_str.cap != 0 {
      lg_str_write_into(&mut (*pack).stdout_str, formatted.as_ptr());
    }
  }
}

fn main() {
  unsafe { // useful main func
    let logs_dir = CString::new("logs").unwrap();
    let formatter: LogFormatterT = myFormatter;

    let config = LoggerConfig {
      local_time: 1,
      print_stdout: 1,
      policy: LgLogPolicy::Drop,
      log_formatter: Some(formatter), // FUCK ALL RUST DEVELOPERS AND GOONERS
    };

    let lg = lg_alloc();
    let ires = lg_init(lg, logs_dir.as_ptr(), config);
    if ires != 1 {
      println!("Logger initialization failed with code {ires}!");
      return;
    }

    // 10k stress-test
    for i in 0..10000 {
      //println!("{i}");
      let fmt = format!("Fuck Rust {:04} times", i);
      let msg = CString::new(fmt).unwrap();
      lg_finfo(msg.as_ptr());
      //lg_ferror(msg.as_ptr());
      //lg_fwarn(msg.as_ptr());
    }
    
    let dres = lg_destroy(lg);
    if dres != 1 {
      println!("Logger destruct failed with code {dres}!");
      return;
    }
    lg_free(lg);
  }
}

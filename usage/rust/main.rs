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
use std::env;

mod liblogger;
use liblogger::*;

// rust doing rust thing
unsafe fn cstr_to_string(cstr: *const c_char) -> String {
  unsafe { CStr::from_ptr(cstr).to_string_lossy().into_owned() }
}

lg_formatter!(myFormatter, |time_str, level, msg, out_type| {
  if out_type == LgOutType::TTY {
    format!("\x1b[36m{}\x1b[0m [{}] {}", time_str, level, msg)
  } else {
    format!("{} [{}] {}", time_str, level, msg)
  }
});

macro_rules! cstr {
  ($s:expr) => {
    concat!($s, "\0").as_ptr() as *const i8
  };
}

fn main() {
  unsafe { // useful main func
    let logs_dir = CString::new("logs").unwrap();
    let formatter: LogFormatterT = myFormatter;

    let mut config = LoggerConfig {
      local_time: 1,
      max_files: 0,
      generate_default_file: 1,
      log_policy: LgLogPolicy::Drop,
      sinks: LgSinks::default(),
      log_formatter: None, //Some(formatter), // FUCK ALL RUST DEVELOPERS AND GOONERS
    };
    lg_append_sink(&mut config, lg_get_stdout(), LgOutType::TTY);
    lg_append_sink(&mut config, lg_fopen(cstr!("some.log")), LgOutType::Net);

    let lg = lg_alloc();
    let ires = lg_init(lg, logs_dir.as_ptr(), config);
    if ires != 1 {
      println!("Logger initialization failed with code {ires}!");
      return;
    }

    let args: Vec<String> = env::args().collect();
    if args.get(1).map(|s| s == "-s").unwrap_or(false) {
      // 10k stress-test
      for i in 0..10000 {
        //println!("{i}");
        let fmt = format!("Fuck Rust {:04} times", i);
        let msg = CString::new(fmt).unwrap();
        lg_finfo(msg.as_ptr());
        //lg_ferror(msg.as_ptr());
        //lg_fwarn(msg.as_ptr());
      }
    } else {
      lg_finfo(cstr!("Hello from Rust!"));
      lg_fwarn(cstr!("Warn from Rust!"));
      lg_ferror(cstr!("Error from Rust!"));
    }

    let dres = lg_destroy(lg);
    if dres != 1 {
      println!("Logger destruct failed with code {dres}!");
      return;
    }
    lg_free(lg);
  }
}

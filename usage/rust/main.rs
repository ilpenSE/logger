// shut the fuck up rust
#![allow(unused_variables)]
#![allow(unused_imports)]
#![allow(dead_code)]

use std::ffi::CString;
use std::os::raw::{c_char, c_int, c_void};
use std::fmt;

mod liblogger;
use liblogger::*;

fn main() {
  let logs_dir = CString::new("logs").unwrap();

  let config = LoggerConfig {
    local_time: 1,
    print_stdout: 1,
    log_formatter: None,
  };
  
	unsafe {
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

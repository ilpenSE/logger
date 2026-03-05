fn main() {
  println!("cargo:rustc-link-search=native=../../build");
  println!("cargo:rustc-link-lib=logger");
  println!("cargo:rustc-link-arg=-Wl,-rpath,../../build");
  println!("cargo:rerun-if-changed=logger.h");
}

fn main() {
  println!("cargo:rustc-link-search=native=../../build/");
  println!("cargo:rustc-link-lib=logger");
  println!("cargo:rerun-if-changed=logger.h");
}

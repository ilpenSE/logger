import { dlopen, FFIType, suffix } from "bun:ffi";

/*
NOTE: BECAUSE OF THE BUN, This doesnt support custom
log_formatter_t logFormatter, and LoggerConfig struct
Just use instances, alloc/free, init/destroy and that's it.
FFIType.ptr is something like void* normally it takes Logger*
*/

const { symbols } = dlopen(`../../build/liblogger.${suffix}`, {
  // Allocator functions
  lg_alloc:    { returns: FFIType.ptr,  args: [] },
  lg_free:     { returns: FFIType.void, args: [FFIType.ptr] },

  // Lifetime functions
  lg_init_flat:{ returns: FFIType.i32,  args: [
      FFIType.ptr, FFIType.cstring, FFIType.i32, FFIType.i32, FFIType.ptr] },
  lg_destroy:  { returns: FFIType.i32,  args: [FFIType.ptr] },

  // Log functions
  lg_finfo:    { returns: FFIType.i32,  args: [FFIType.cstring] },
  lg_ferror:   { returns: FFIType.i32,  args: [FFIType.cstring] },
  lg_fwarn:    { returns: FFIType.i32,  args: [FFIType.cstring] },
});

export default symbols;

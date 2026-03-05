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
      FFIType.ptr, FFIType.cstring, FFIType.i32, FFIType.i32, FFIType.i32, FFIType.ptr] },
  lg_destroy:  { returns: FFIType.i32,  args: [FFIType.ptr] },

  // Log functions
  // Implicit instances
  lg_flog:     { returns: FFIType.i32,  args: [FFIType.i32, FFIType.cstring] },
  lg_finfo:    { returns: FFIType.i32,  args: [FFIType.cstring] },
  lg_ferror:   { returns: FFIType.i32,  args: [FFIType.cstring] },
  lg_fwarn:    { returns: FFIType.i32,  args: [FFIType.cstring] },

  // Explicit instances
  lg_flogi:    { returns: FFIType.i32,  args: [FFIType.ptr, FFIType.i32, FFIType.cstring] },
  lg_finfoi:   { returns: FFIType.i32,  args: [FFIType.ptr, FFIType.cstring] },
  lg_ferrori:  { returns: FFIType.i32,  args: [FFIType.ptr, FFIType.cstring] },
  lg_fwarni:   { returns: FFIType.i32,  args: [FFIType.ptr, FFIType.cstring] },
});

// Log levels
export const LG_INFO    = 1 << 0;
export const LG_ERROR   = 1 << 1;
export const LG_WARNING = 1 << 2;
export const LG_CUSTOM  = 1 << 3;

// Log Policies
export const LG_DROP           = 1 << 0;
export const LG_SMASH_OLDEST   = 1 << 1;

export default symbols;

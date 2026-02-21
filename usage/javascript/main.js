import logger, { LG_DROP } from "./liblogger.js";

function cstr(string) {
  return Buffer.from(string + "\0");
}

const lg = logger.lg_alloc();
// isLocaltime and isStdout provided, custom log func cannot be implemented
if (logger.lg_init_flat(lg, cstr("logs"), true, true, LG_DROP) !== 1) {
  console.error("Logger init failed");
  process.exit(1);
}

logger.lg_finfo(cstr("Hello from Bun!"));
logger.lg_ferror(cstr("Error from Bun!"));
logger.lg_fwarn(cstr("Warning from Bun!"));

if (logger.lg_destroy(lg) !== 1) {
  console.error("Logger destroy failed");
}
logger.lg_free(lg);

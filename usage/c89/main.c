#include <stdio.h>
#include <logger.h>

/* Must be pointer because sizeof(Logger) is unknown */
static Logger* lg;

int myFormatter(const char* time_str, LgLogLevel level,
               const char* msg, uint32_t needed, LgMsgPack pack) {
  LgString* file_str = &pack[LG_OUT_FILE];
  LgString* tty_str = &pack[LG_OUT_TTY];
  LgString* net_str = &pack[LG_OUT_NET];
  const char* level_str = lg_lvl_to_str(level);

  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_TTY)) {
    lg_str_format_into(
      tty_str,
      "%s \e[6;36m%s\e[0m: %s\n",
      time_str, level_str, msg
      );
  }

  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_FILE)) {
    lg_str_format_into(
      file_str,
      "%s %s: %s\n",
      time_str, level_str, msg
      );
  }
  
  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_NET)) {
    lg_str_format_into(
      net_str,
      "%s [NET] %s: %s\n",
      time_str, level_str, msg
      );
  }
  return 1;
}

int main() {
  lg = lg_alloc();
  LoggerConfig cfg = lg_get_defaults();
  cfg.logFormatter = myFormatter;
  lg_append_sink(&cfg, stderr, LG_OUT_NET);

  lg_init(lg, "logs", cfg);
  lg_info("Hello from C89!");
  lg_destroy(lg);
  return 0;
}

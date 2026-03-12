#define LOGGER_IMPLEMENTATION
#define LOGGER_DEBUG
#define LOGGER_MINIFY_PREFIXES
#include "logger.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

void do_something();

// custom formatter usage (dont forget to add \n)
int myFormatter(const char* time_str, LgLogLevel level,
                const char* msg, uint32_t needed, LgMsgPack pack) {
  LgString* tty_str  = &pack[LG_OUT_TTY];
  LgString* file_str = &pack[LG_OUT_FILE];
  LgString* net_str = &pack[LG_OUT_NET];
  const char* level_str = lg_lvl_to_str(level);

  // equivalent to: needed & (1u << LG_OUT_TTY)
  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_TTY)) {
    lg_str_format_into(tty_str, "%s/%s: %s\n", time_str, level_str, msg);
  }
  
  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_FILE)) {
    lg_str_format_into(file_str, "%s/%s: %s\n", time_str, level_str, msg);
  }

  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_NET)) {
    lg_str_format_into(net_str,
                       "{timestamp:%s, level:%s, msg:%s}\n",
                       time_str, level_str, msg);
  }
  return true;
}

Logger* lg;
int main() {
  FILE* customFile = fopen("log.log", "wb");
  if (customFile == NULL) return 1;

  lg = lg_alloc();

  LoggerConfig conf = {
    .localTime = true,
    .generateDefaultFile = true,
    .logPolicy = LG_DROP,
    .maxFiles = 0,
    .logFormatter = myFormatter,
  };
  lg_append_sink(&conf, stderr, LG_OUT_NET);
  lg_append_sink(&conf, stdout, LG_OUT_TTY);
  lg_append_sink(&conf, customFile, LG_OUT_FILE);

  if (!lg_init(lg, "logs", conf)) {
    perror("logs init failed");
    return -1;
  }

  lg_info("the %s", "informatics");
  lg_error("the errormatics");
  lg_warn("the warningmatics");

  do_something();

  if (!lg_destroy(lg)) {
    perror("logger destruct failed");
    return -1;
  }

  fprintf(stderr, "stderr message\n");
  fprintf(stdout, "stdout message\n");

  lg_free(lg);
  return 0;
}

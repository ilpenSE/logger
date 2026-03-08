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
int myFormatter(int isLocalTime, LgLogLevel level,
                const char* msg, uint32_t needed, LgMsgPack pack) {
  char time_str[LOGGER_TIME_STR_SIZE];
  if (!lg_get_time_str(time_str, isLocalTime)) return false;

  LgString* stdout_str = &pack[LG_OUT_TTY];
  LgString* file_str = &pack[LG_OUT_FILE];

  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_TTY)) {
    lg_str_format_into(stdout_str, "%s/%s: %s\n", time_str, lg_lvl_to_str(level), msg);
  }
  
  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_FILE)) {
    lg_str_format_into(file_str, "%s/%s: %s\n", time_str, lg_lvl_to_str(level), msg);
  }
  return true;
}

Logger* lg;
int main() {
  lg = lg_alloc();

  LoggerConfig conf = {
    .localTime = true,
    .printStdout = true,
    .logPolicy = LG_DROP,
    .maxFiles = 0,
    .logFormatter = NULL,
  };

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

  lg_free(lg);
  return 0;
}

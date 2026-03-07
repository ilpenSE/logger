#define LOGGER_IMPLEMENTATION
#define LOGGER_DEBUG
#define LOGGER_MINIFY_PREFIXES
#include "logger.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>

void do_something();

// custom formatter usage (dont forget to add \n)
int myFormatter(const int isLocalTime, const LgLogLevel level,
                const char* msg, LgMsgPack* pack) {
  char time_str[LOGGER_TIME_STR_SIZE];
  if (!lg_get_time_str(time_str, isLocalTime)) return false;

  if (pack->stdout_str.data) {
    lg_str_format_into(&pack->stdout_str, "%s/%s: %s\n", time_str, lg_lvl_to_str(level), msg);
  }
  
  if (pack->file_str.data) {
    lg_str_format_into(&pack->file_str, "%s/%s: %s\n", time_str, lg_lvl_to_str(level), msg);
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
    .maxFiles = 3,
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

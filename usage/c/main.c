#define LOGGER_IMPLEMENTATION
//#define LOGGER_DEBUG
#define LOGGER_MINIFY_PREFIXES
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

// custom formatter usage (dont forget to add \n)
void myFormatter(const char* time_str, const lg_log_level level,
                 const char* msg, lg_msg_pack* pack) {
	// smth like: 2026.01.22-00.15.20.810/INFO: stb lets gooo
  if (pack->stdout_str.data && pack->stdout_str.cap != 0) {
    lg_str_format_into(&pack->stdout_str, "%s/%s: %s\n", time_str, lg_lvl_to_str(level), msg);
  }
  
  if (pack->file_str.data && pack->file_str.cap != 0) {
    lg_str_format_into(&pack->file_str, "%s/%s: %s\n", time_str, lg_lvl_to_str(level), msg);
  }
}

Logger* lg;
int main() {
	lg = lg_alloc();

	LoggerConfig conf = {
		.localTime = true,
    .printStdout = true,
		.policy = LG_DROP,
    .logFormatter = NULL
	};

	if (!lg_init(lg, "logs", conf)) {
		perror("logs init failed");
		return -1;
	}

	lg_info("the informatics");
  lg_error("the errormatics");
  lg_warn("the warningmatics");

	if (!lg_destroy(lg)) {
		perror("logger destruct failed");
		return -1;
	}

  lg_free(lg);
	return 0;
}

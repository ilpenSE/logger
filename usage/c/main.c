#define LOGGER_IMPLEMENTATION
#define LOGGER_DEBUG
#define LOGGER_STRIP_PREFIXES
#define LOGGER_MINIFY_PREFIXES
#include "logger.h"

#include <stdio.h>
#include <stddef.h>

// custom formatter usage (dont forget to add \n)
int myFormatter(const char* time_str, const char* level, const char* msg,
								char* out, size_t size) {
	return snprintf(out, size, "%s/%s: %s\n", time_str, level, msg);
	// smth like: 2026.01.22-00.15.20.810/INFO: stb lets gooo
}

int main() {
	LoggerConfig conf = {
		.localTime = true,
    .printStdout = true,
    .logFormatter = NULL
	};
	if (lg_init("logs", conf) != 1) {
		perror("logs init failed");
		return -1;
	}

	lg_info("the informatics");
	lg_custom("customized");
	linfo("minified info");
	warn("stripped warning");
	error("stripped error");
  
	if (lg_destruct() != 1) {
		perror("logger destruct failed");
		return -1;
	}
	return 0;
}

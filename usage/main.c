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

int main(int argc, char** argv) {
  char* logs_path;
	if (argc < 2) logs_path = "logs";
	else logs_path = argv[1];

	LoggerConfig conf = {
		.localTime = 1,
    .printStdout = 1,
    .logFormatter = NULL
	};
	if (lg_init(logs_path, conf) != 1) {
		perror("logs init failed");
		return -1;
	}

	lg_info("the informatics");
	lg_custom("customized");
	linfo("minified info");
	warn("stripped warning");
	error("stripped error");
// output: (if commented LG_DEBUG's are uncommented)
// ../logger.h:551: INFO: Producer appended to ring: "2026.01.27-18.29.33.075 [INFO] the informatics
// ", length = 47
// ../logger.h:557: INFO: Producer invoked writer thread
// ../logger.h:551: INFO: Producer appended to ring: "2026.01.27-18.29.33.075 [CUSTOM] customized
// ", length = 44
// ../logger.h:557: INFO: Producer invoked writer thread
// ../logger.h:374: INFO: Writer recieved "2026.01.27-18.29.33.075 [INFO] the informatics
// ", length = 47
// ../logger.h:551: INFO: Producer appended to ring: "2026.01.27-18.29.33.075 [INFO] minified info
// ", length = 45
// ../logger.h:557: INFO: Producer invoked writer thread
// 2026.01.27-18.29.33.075 [INFO] the informatics
// ../logger.h:379: INFO: Writer wrote "2026.01.27-18.29.33.075 [INFO] the informatics
// ", length = 47
// ../logger.h:374: INFO: Writer recieved "2026.01.27-18.29.33.075 [CUSTOM] customized
// ", length = 44
// 2026.01.27-18.29.33.075 [CUSTOM] customized
// ../logger.h:379: INFO: Writer wrote "2026.01.27-18.29.33.075 [CUSTOM] customized
// ", length = 44
// ../logger.h:374: INFO: Writer recieved "2026.01.27-18.29.33.075 [INFO] minified info
// ", length = 45
// 2026.01.27-18.29.33.075 [INFO] minified info
// ../logger.h:551: INFO: Producer appended to ring: "2026.01.27-18.29.33.076 [WARNING] stripped warning
// ", length = 51
// ../logger.h:557: INFO: Producer invoked writer thread
// ../logger.h:551: INFO: Producer appended to ring: "2026.01.27-18.29.33.076 [ERROR] stripped error
// ", length = 47
// ../logger.h:557: INFO: Producer invoked writer thread
// ../logger.h:379: INFO: Writer wrote "2026.01.27-18.29.33.075 [INFO] minified info
// ", length = 45
// ../logger.h:387: INFO: Writer thread is exiting
// this is the basic behavior of async logger
  
	if (lg_destruct() != 1) {
		perror("logger destruct failed");
		return -1;
	}
	return 0;
}

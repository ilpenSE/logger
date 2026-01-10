#include <stdio.h>

#define LOGGER_STRIP_PREFIXES
#define LOGGER_MINIFY_PREFIXES
#include "logger.h"

int main() {
	// init logger before using logger
	if (!lg_init("logs", 1, 1)) {
		perror("logs init failed bru\n");
		return -1;
	}

	// straight
	lg_info("Hello from lg_info!");
	lg_error("Hello from lg_error!");
	lg_warn("Hello from lg_warn!");

	// stripped
	info("Hello from stripped info!");
	error("Hello from stripped error!");
	warn("Hello from stripped warn!");

	// minified
	linfo("Hello from minified info!");
	lerror("Hello from minified error!");
	lwarn("Hello from minified warn!");

	// always destruct it when you're finished with logger
	if (!lg_destruct()) {
		perror("logs destruct faile brrrr\n");
		return -1;
	}
	return 0;
}

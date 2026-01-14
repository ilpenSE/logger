#define LOGGER_IMPLEMENTATION
#define LOGGER_DEBUG
#include "logger.h"

#include <stdio.h>

int main(int argc, char** argv) {
	char* logs_path;
	if (argc < 2) logs_path = "logs";
	else logs_path = argv[1];
	
	if (!lg_init(logs_path, 1)) {
		perror("logs init failed");
		return -1;
	}

	lg_info("stb lets gooo");
	lg_error("nga err");
	lg_warn("tame impala's warning");

	if (!lg_destruct()) {
		perror("logger destruct failed");
		return -1;
	}
	return 0;
}

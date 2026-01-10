#include <stdio.h>

#define LOGGER_STRIP_PREFIXES
#define LOGGER_MINIFY_PREFIXES
#include "logger.h"

void simple_test() {
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
}

void use_after_destruct() {
	info("Log before destruct");
	if (!lg_destruct()) return;
	info("Log after destruct");
}

void hello() {
	printf("======== LOGGER C USAGE ========\n");
	printf("[0] Simple test\n");
	printf("[1] Use-After-Destruct test\n");
	printf("[2] Print this message\n");
	printf("[3] Exit\n");
}

int main() {
	// init logger before using logger
	if (!lg_init("logs", 1, 1)) {
		perror("logs init failed bru\n");
		return -1;
	}

	int op = 0;
	hello();
	while (1) {
		if (!scanf("%d", &op)) {
			printf("Please enter a valid operation!\n");
			// invalid entry, clear stdin
			// this time it wont print please enter a valid op infinitely
			int c;
			while ((c = getchar()) != '\n' && c != EOF) { } 
			continue;
		}
		switch(op) {
		case 0:
			simple_test();
			break;
		case 1:
			use_after_destruct();
			break;
		case 2:
			hello();
			break;
		case 3:
			goto exit;
		default:
			printf("Please enter a valid operation!\n");
			break;
		}
	}
exit:
	// always destruct it when you're finished with logger
	if (!lg_destruct()) {
		perror("logs destruct faile brrrr\n");
		return -1;
	}
	return 0;
}

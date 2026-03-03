# This makefile only works on UNIX/Linux
CC = gcc
CFLAGS = -std=c99 -x c -DLOGGER_IMPLEMENTATION -pthread -fPIC -Wall -Wextra -shared

BUILD_FOLDER = build
HEADER = logger.h
SHARED_OBJECT ?= $(BUILD_FOLDER)/liblogger.so
debug ?= 0

ifeq ($(debug),1)
	CFLAGS += -DLOGGER_DEBUG -g -O0
else
	CFLAGS += -O2
endif

all: $(SHARED_OBJECT)

# Build folder directory making
$(BUILD_FOLDER):
	mkdir -p $(BUILD_FOLDER)

# Generating shared object
$(SHARED_OBJECT): $(HEADER) | $(BUILD_FOLDER)
	$(CC) $(CFLAGS) -o $(SHARED_OBJECT) $(HEADER)

clean:
	rm -rf $(BUILD_FOLDER)

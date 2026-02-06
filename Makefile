# This makefile only works on UNIX/Linux
# It can cross-compile windows DLL (if you have x86_64-w64-mingw32-gcc)
CC ?= gcc
CFLAGS = -x c -DLOGGER_IMPLEMENTATION -pthread -fPIC -Wall -Wextra

BUILD_FOLDER = build
HEADER = logger.h
OBJECT = $(BUILD_FOLDER)/logger.o
SHARED_OBJECT ?= $(BUILD_FOLDER)/liblogger.so
debug ?= 0
windows ?= 0

# Debug config
ifeq ($(debug),1)
	CFLAGS += -DLOGGER_DEBUG -g -O0
else
	CFLAGS += -O2
endif

# Windows config
ifeq ($(windows),1)
	CC = x86_64-w64-mingw32-gcc
	CFLAGS += -DLOGGER_BUILD
	SHARED_OBJECT = $(BUILD_FOLDER)/logger.dll
endif

all: $(SHARED_OBJECT)

# Build folder directory making
$(BUILD_FOLDER):
	mkdir -p $(BUILD_FOLDER)

# Generating object (.o) file
$(OBJECT): $(HEADER) | $(BUILD_FOLDER)
	$(CC) $(CFLAGS) -c $(HEADER) -o $(OBJECT)

# Generating shared object (.so/.dll) file
$(SHARED_OBJECT): $(OBJECT) | $(BUILD_FOLDER)
	$(CC) -shared -pthread -o $(SHARED_OBJECT) $(OBJECT)

clean:
	rm -rf $(BUILD_FOLDER)

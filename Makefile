CC = gcc
CFLAGS = -x c -DLOGGER_IMPLEMENTATION -pthread -fPIC

BUILD_FOLDER = build
HEADER = logger.h
OBJECT = $(BUILD_FOLDER)/logger.o
SHARED_OBJECT = $(BUILD_FOLDER)/liblogger.so
debug ?= 0

ifeq ($(debug),1)
CFLAGS += -DLOGGER_DEBUG -g -O0
else
CFLAGS += -O2
endif

all: $(SHARED_OBJECT)

$(BUILD_FOLDER):
	mkdir -p $(BUILD_FOLDER)

$(OBJECT): $(HEADER) | $(BUILD_FOLDER)
	$(CC) $(CFLAGS) -c $(HEADER) -o $(OBJECT)

$(SHARED_OBJECT): $(OBJECT)
	$(CC) -shared -pthread -o $(SHARED_OBJECT) $(OBJECT)

clean:
	rm -rf $(BUILD_FOLDER)

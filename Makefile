# This makefile only works on UNIX/Linux
CC = gcc
CFLAGS = -std=c11 -x c -DLOGGER_IMPLEMENTATION -pthread -fPIC -Wall -Wextra

BUILD = build
HEADER = logger.h
OBJECT = $(BUILD)/logger.o
DYNAMIC_LIB ?= $(BUILD)/liblogger.so
STATIC_LIB ?= $(BUILD)/liblogger.a

debug ?= 0

ifeq ($(debug),1)
	CFLAGS += -DLOGGER_DEBUG -g -O0
else
	CFLAGS += -O2
endif

all: $(DYNAMIC_LIB) $(STATIC_LIB)

# Build folder directory making
$(BUILD):
	mkdir -p $(BUILD)

$(OBJECT): $(HEADER) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $(OBJECT)

$(DYNAMIC_LIB): $(OBJECT) | $(BUILD)
	$(CC) -shared -o $@ $<

$(STATIC_LIB): $(OBJECT) | $(BUILD)
	ar rcs $@ $(OBJECT)

clean:
	rm -rf $(BUILD)

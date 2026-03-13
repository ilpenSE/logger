# This makefile only works on UNIX/Linux
CC = gcc
CFLAGS = -std=c11 -x c -DLOGGER_IMPLEMENTATION -pthread -fPIC -Wall -Wextra

BUILD = build
HEADER = logger.h
NOIMPL_HEADER = $(BUILD)/logger.h
OBJECT = $(BUILD)/logger.o
DYNAMIC_LIB ?= $(BUILD)/liblogger.so
STATIC_LIB ?= $(BUILD)/liblogger.a

debug ?= 0

ifeq ($(debug),1)
	CFLAGS += -DLOGGER_DEBUG -g -O0
else
	CFLAGS += -O2
endif

all: $(DYNAMIC_LIB) $(STATIC_LIB) $(NOIMPL_HEADER)

# Build folder directory making
$(BUILD):
	mkdir -p $(BUILD)

$(OBJECT): $(HEADER) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $(OBJECT)

$(DYNAMIC_LIB): $(OBJECT) | $(BUILD)
	$(CC) -shared -o $@ $<

$(STATIC_LIB): $(OBJECT) | $(BUILD)
	ar rcs $@ $(OBJECT)

$(NOIMPL_HEADER): $(HEADER) | $(BUILD)
	awk ' \
		BEGIN { \
			print "/*"; \
			print "  This file was generated automatically"; \
			print "  It doesnt have implementation"; \
			print "  Compatible with >=C89 or >=C++98"; \
			print "*/"; \
			print ""; \
		} \
		/IMPLEMENTATION BEGIN/ {skip=1} \
		/IMPLEMENTATION END/ {skip=0; next} \
		!skip \
	' $(HEADER) > $(NOIMPL_HEADER)

clean:
	rm -rf $(BUILD)

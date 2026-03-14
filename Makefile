# This makefile only works on Linux, OSX and Windows with MinGW
# For MSVC, Go to Makefile.win
# Usage:
#   make                       -> Linux x86-64 (gcc)
#   make arch=aarch64          -> Linux ARM64
#   make os=osx                -> macOS ARM64 (Apple Silicon)
#   make os=osx arch=x86_64    -> macOS x86-64
#   make os=mingw              -> Windows x86-64 (mingw)
#   make os=mingw arch=aarch64 -> Windows ARM64 (llvm-mingw)

CC ?= gcc
CFLAGS = -std=c11 -x c -DLOGGER_IMPLEMENTATION -pthread -fPIC -Wall -Wextra
DYNAMIC_LIB_EXT ?= so
BUILD = build
HEADER = logger.h
NOIMPL_HEADER = $(BUILD)/logger.h
OBJECT = $(BUILD)/logger.o
DYNAMIC_LIB ?= $(BUILD)/liblogger.$(DYNAMIC_LIB_EXT)
STATIC_LIB ?= $(BUILD)/liblogger.a
debug ?= 0

# os and arch parameter dispatch
ifeq ($(os),osx)
	DYNAMIC_LIB_EXT = dylib
ifeq ($(arch),x86_64)
		CC = x86_64-apple-darwin25.1-clang
else
		CC = aarch64-apple-darwin25.1-clang
endif
else ifeq ($(os),mingw) # MinGW, for MSVC go to Makefile.win
	DYNAMIC_LIB_EXT = dll
ifeq ($(arch),aarch64)
		CC = aarch64-w64-mingw32-gcc
else
		CC = x86_64-w64-mingw32-gcc
endif
else
ifeq ($(arch),aarch64)
		CC = aarch64-linux-gnu-gcc
endif
endif

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

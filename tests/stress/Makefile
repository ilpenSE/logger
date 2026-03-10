CC := gcc
CFLAGS := -Wall -Wextra -I../../ -ggdb

TARGET := app
SRC := stress.c

all: $(TARGET)

$(TARGET): $(SRC) ../../logger.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean

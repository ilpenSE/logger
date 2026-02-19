CC := gcc
CFLAGS := -Wall -Wextra -I../..

TARGET := app
SRC := main.c

all: $(TARGET)

$(TARGET): $(SRC) ../../logger.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean

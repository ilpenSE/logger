CC := gcc
CFLAGS := -Wall -Wextra -O2 -I../../

TARGET := app
SRC := main.c multithread.c singlethread.c

all: $(TARGET)

$(TARGET): $(SRC) ../../logger.h utils.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean

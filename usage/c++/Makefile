CXX := g++
CXXFLAGS := -O2 -std=c++17 -Wall -Wextra -I../..
TARGET := app
SRC := main.cpp

all: $(TARGET)

$(TARGET): $(SRC) ../../logger.h
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean

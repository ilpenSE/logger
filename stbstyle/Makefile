CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -I../include/stbstyle
TARGET := app
SRC := main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean

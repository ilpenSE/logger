CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -I.
LDFLAGS := -L.

VERBOSE ?= 0   # default 0

ifeq ($(VERBOSE),1)
	TARGET := appv
	LDFLAGS += -lloggerv
else
	TARGET := app
	LDFLAGS += -llogger
endif

RPATH := -Wl,-rpath,'$$ORIGIN'

SRC := main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS) $(RPATH)

clean:
	rm -f $(TARGET)

.PHONY: all clean


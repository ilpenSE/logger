CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -I../include

VERBOSE ?= 0

ifeq ($(VERBOSE),1)
	LDFLAGS := -L../libs/verbosed -lloggerv
	TARGET := appv
	RPATH := -Wl,-rpath,'$$ORIGIN/../libs/verbosed'
else
	LDFLAGS := -L../libs -llogger
	TARGET := app
	RPATH := -Wl,-rpath,'$$ORIGIN/../libs'
endif

SRC := main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS) $(RPATH)

clean:
	rm -f $(TARGET)

.PHONY: all clean

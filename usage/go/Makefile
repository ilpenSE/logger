GOC = go

TARGET = app
SRC = main.go
HEADER = ../../logger.h
LIBLOGGERGO = logger/liblogger.go
LOGGER_TESTGO = logger/logger_test.go

all: $(TARGET)

$(TARGET): $(SRC) $(HEADER)
	$(GOC) build $(GOFLAGS) -o $(TARGET) $(SRC)

test: $(HEADER)
	$(GOC) test $(LOGGER_TESTGO) $(LIBLOGGERGO)

run: $(SRC) $(HEADER) $(LIBLOGGERGO)
	$(GOC) run $(SRC)

clean:
	rm -rf $(TARGET)

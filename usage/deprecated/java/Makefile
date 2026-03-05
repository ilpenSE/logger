# You have to download jna.jar:
# wget https://repo1.maven.org/maven2/net/java/dev/jna/jna/5.14.0/jna-5.14.0.jar -O jna.jar

JNA = jna.jar
BUILD = build
SRC = Main.java LoggerLib.java
LIB_PATH = ../../build

all: run

$(BUILD):
	mkdir -p $(BUILD)

compile: $(BUILD)
	javac -cp .:$(JNA) -d $(BUILD) $(SRC)

run: compile
	LD_LIBRARY_PATH=$(LIB_PATH):$$LD_LIBRARY_PATH java -cp $(BUILD):$(JNA) -Djava.library.path=$(LIB_PATH) Main

clean:
	rm -rf $(BUILD)

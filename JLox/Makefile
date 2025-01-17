# Variables
SRC_DIR := src
TEST_DIR := test
BUILD_DIR := build
MAIN_CLASS := com.xinlong.JLox.Lox

# Find all .java files in the source directory
SOURCES := $(shell find $(SRC_DIR) -name "*.java")

# Find the relative path without extension of all .lox files in the test directory and subdirectories
TESTS := $(shell find $(TEST_DIR) -name "*.lox" | sed 's/\.lox//')

# Create a list of .class files from the .java files
CLASSES := $(SOURCES:$(SRC_DIR)/%.java=$(BUILD_DIR)/%.class)

# Default target
all: $(BUILD_DIR) build-src # $(CLASSES)

# Rule to create the build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Rule to compile .java files to .class files
$(BUILD_DIR)/%.class: $(SRC_DIR)/%.java
	javac -d $(BUILD_DIR) $<

build-src: $(SOURCES)
	javac -d $(BUILD_DIR) $(SOURCES)

# Clean target to remove build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Run the main class
run: all
	java -cp $(BUILD_DIR) $(MAIN_CLASS)

$(TESTS): all $(TESTS).lox
	java -cp $(BUILD_DIR) $(MAIN_CLASS) $@.lox

test: all $(TESTS)
	echo java -cp $(BUILD_DIR) com.xinlong.JLox.Lox test.lox

.PHONY: all clean run
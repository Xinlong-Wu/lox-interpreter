# Variables
SRC_DIR := src
BUILD_DIR := build
MAIN_CLASS := com.xinlong.JLox.Lox

# Find all .java files in the source directory
SOURCES := $(shell find $(SRC_DIR) -name "*.java")

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

.PHONY: all clean run
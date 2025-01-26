# Variables
SRC_DIR := JLox
TOOLS_DIR := tools
TEST_DIR := test
BUILD_DIR := build
TOOLS_BUILD_DIR := build/tools
MAIN_CLASS := JLox.Lox

# Find all .java files in the source directory and subdirectories
SOURCES :=	$(shell find $(SRC_DIR) -name "*.java")
TOOLS_SOURCES :=	$(shell find $(TOOLS_DIR) -name "*.java")

# Find the relative path without extension of all .lox files in the test directory and subdirectories
TESTS := $(shell find $(TEST_DIR) -name "*.lox" | sed 's/\.lox//')

# Create a list of .class files from the .java files
CLASSES := $(SOURCES:$(SRC_DIR)/%.java=$(BUILD_DIR)/%.class)

# Default target
all: $(BUILD_DIR) GenerateAst build-src #$(CLASSES)

# Rule to create the build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TOOLS_BUILD_DIR):
	mkdir -p $(TOOLS_BUILD_DIR)

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

$(TESTS): all $@
	java -cp $(BUILD_DIR) $(MAIN_CLASS) $@.lox

build-tools: $(TOOLS_SOURCES)
	javac -d $(TOOLS_BUILD_DIR) $(TOOLS_SOURCES)

GenerateAst: $(TOOLS_BUILD_DIR) build-tools 
	java -cp $(TOOLS_BUILD_DIR) tools.$@ $(SRC_DIR)/Expression

test: all $(TESTS)

.PHONY: all clean run
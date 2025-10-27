# Compiler settings
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g
INCLUDES := -I./includes

# Directories
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
OBJ_DIR := $(BUILD_DIR)/obj

# Find all source files recursively
SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')

# Generate object file paths
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))

# Generate dependency files
DEPS := $(OBJECTS:.o=.d)

# Targets for each Part (you can customize these)
PART_II_SRCS := $(SRC_DIR)/Part_II/bitstream.cpp $(SRC_DIR)/Part_II/golomb.cpp
PART_II_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(PART_II_SRCS))

# Default target
all: directories $(BIN_DIR)/part_ii_test

# Create necessary directories
directories:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)/Part_I
	@mkdir -p $(OBJ_DIR)/Part_II
	@mkdir -p $(OBJ_DIR)/Part_III
	@mkdir -p $(OBJ_DIR)/Part_IV
	@mkdir -p $(OBJ_DIR)/Part_V

# Build Part II test executable (example)
$(BIN_DIR)/part_ii_test: $(PART_II_OBJS)
	@echo "Linking $@..."
	@$(CXX) $(CXXFLAGS) $^ -o $@
	@echo "Build complete: $@"

# Generic rule for compiling .cpp to .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Include dependency files
-include $(DEPS)

# Clean build artifacts
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean complete"

# Phony targets
.PHONY: all clean directories

# Help target
help:
	@echo "Available targets:"
	@echo "  all        - Build all targets (default)"
	@echo "  clean      - Remove all build artifacts"
	@echo "  directories - Create necessary directories"
	@echo "  help       - Show this help message"

# Compiler settings
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O3 -g
INCLUDES := -Iincludes

# Directories
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
OBJ_DIR := $(BUILD_DIR)/obj
DEP_DIR := $(BUILD_DIR)/deps

# Find all source files recursively
ALL_CPP_SOURCES := $(shell find $(SRC_DIR) -name '*.cpp' 2>/dev/null)

# Separate by category
IMAGE_SOURCES := $(filter $(SRC_DIR)/image/%,$(ALL_CPP_SOURCES))
CODEC_SOURCES := $(filter $(SRC_DIR)/codec/%,$(ALL_CPP_SOURCES))

# Top-level common sources (bitstream.cpp, golomb.cpp) - only if they exist
COMMON_SOURCES := $(wildcard $(SRC_DIR)/bitstream.cpp $(SRC_DIR)/golomb.cpp)

# Part sources (if they exist)
PART_SOURCES := $(filter $(SRC_DIR)/Part_%,$(ALL_CPP_SOURCES))

# All non-main sources
ALL_SOURCES := $(filter-out %/main.cpp,$(ALL_CPP_SOURCES))

# Object files
ALL_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(ALL_SOURCES))
DEPS := $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$(ALL_OBJECTS))

# Find all main.cpp files
MAIN_SOURCES := $(filter %/main.cpp,$(ALL_CPP_SOURCES))
MAIN_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(MAIN_SOURCES))

# Find all non-main object files to link with
NON_MAIN_OBJECTS := $(filter-out %/main.o,$(ALL_OBJECTS))

# Define executables explicitly based on your structure
# For src/image/main.cpp -> bin/image_tool
EXECUTABLES := $(BIN_DIR)/image_tool

# Default target
all: directories $(EXECUTABLES)

# Create necessary directories dynamically
directories:
	@mkdir -p $(BIN_DIR)
	@for dir in $$(find $(SRC_DIR) -type d 2>/dev/null); do \
		rel_dir=$${dir#$(SRC_DIR)/}; \
		mkdir -p $(OBJ_DIR)/$$rel_dir $(DEP_DIR)/$$rel_dir; \
	done

# Generic rule for compiling .cpp to .o with automatic dependency generation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	@mkdir -p $(patsubst $(OBJ_DIR)/%,$(DEP_DIR)/%,$(dir $@))
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -MF $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@) -c $< -o $@

# Explicit linking rule for image_tool
$(BIN_DIR)/image_tool: $(OBJ_DIR)/image/main.o $(NON_MAIN_OBJECTS)
	@echo "Linking $@..."
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $^ -o $@
	@echo "Build complete: $@"

# Include dependency files
-include $(DEPS)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean complete"

# Clean and rebuild
rebuild: clean all

# Show what files were found
debug:
	@echo "=== Source Files Found ==="
	@echo ""
	@echo "IMAGE_SOURCES ($(words $(IMAGE_SOURCES)) files):"
	@$(foreach src,$(IMAGE_SOURCES),echo "  $(src)";)
	@echo ""
	@echo "CODEC_SOURCES ($(words $(CODEC_SOURCES)) files):"
	@$(foreach src,$(CODEC_SOURCES),echo "  $(src)";)
	@echo ""
	@echo "COMMON_SOURCES ($(words $(COMMON_SOURCES)) files):"
	@$(foreach src,$(COMMON_SOURCES),echo "  $(src)";)
	@echo ""
	@echo "PART_SOURCES ($(words $(PART_SOURCES)) files):"
	@$(foreach src,$(PART_SOURCES),echo "  $(src)";)
	@echo ""
	@echo "MAIN_SOURCES ($(words $(MAIN_SOURCES)) files):"
	@$(foreach src,$(MAIN_SOURCES),echo "  $(src)";)
	@echo ""
	@echo "=== Build Configuration ==="
	@echo "ALL_OBJECTS ($(words $(ALL_OBJECTS)) files):"
	@$(foreach obj,$(ALL_OBJECTS),echo "  $(obj)";)
	@echo ""
	@echo "MAIN_OBJECTS ($(words $(MAIN_OBJECTS)) files):"
	@$(foreach obj,$(MAIN_OBJECTS),echo "  $(obj)";)
	@echo ""
	@echo "NON_MAIN_OBJECTS ($(words $(NON_MAIN_OBJECTS)) files):"
	@$(foreach obj,$(NON_MAIN_OBJECTS),echo "  $(obj)";)
	@echo ""
	@echo "EXECUTABLES ($(words $(EXECUTABLES)) files):"
	@$(foreach exec,$(EXECUTABLES),echo "  $(exec)";)

# Show build info
info:
	@echo "Build Configuration:"
	@echo "  CXX:        $(CXX)"
	@echo "  CXXFLAGS:   $(CXXFLAGS)"
	@echo "  INCLUDES:   $(INCLUDES)"
	@echo "  SRC_DIR:    $(SRC_DIR)"
	@echo "  BUILD_DIR:  $(BUILD_DIR)"
	@echo "  BIN_DIR:    $(BIN_DIR)"
	@echo ""
	@echo "Source Files Found:"
	@echo "  Total:      $(words $(ALL_CPP_SOURCES)) files"
	@echo "  Image:      $(words $(IMAGE_SOURCES)) files"
	@echo "  Codec:      $(words $(CODEC_SOURCES)) files"
	@echo "  Common:     $(words $(COMMON_SOURCES)) files"
	@echo "  Part:       $(words $(PART_SOURCES)) files"
	@echo "  Main:       $(words $(MAIN_SOURCES)) files"
	@echo ""
	@echo "Executables to build:"
	@$(foreach exec,$(EXECUTABLES),echo "  $(exec)";)

# Run tests
test: all
	@echo "Running tests..."
	@for exe in $(BIN_DIR)/*; do \
		if [ -x "$$exe" ]; then \
			echo "Running $$exe..."; \
			$$exe || true; \
		fi; \
	done

# Install (optional - for system-wide installation)
install: all
	@echo "Installing executables..."
	@mkdir -p $(DESTDIR)/usr/local/bin
	@cp $(BIN_DIR)/* $(DESTDIR)/usr/local/bin/ 2>/dev/null || true
	@echo "Installation complete"

# Phony targets
.PHONY: all clean directories rebuild install test info help debug

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build all targets (default)"
	@echo "  clean        - Remove all build artifacts"
	@echo "  rebuild      - Clean and rebuild everything"
	@echo "  directories  - Create necessary directories"
	@echo "  debug        - Show detected source files (detailed)"
	@echo "  info         - Show build configuration (summary)"
	@echo "  install      - Install executables to system"
	@echo "  test         - Run all built executables"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Example usage:"
	@echo "  make debug        # See what files are detected"
	@echo "  make info         # Quick summary"
	@echo "  make              # Build everything"
	@echo "  make clean        # Clean build files"
	@echo "  make rebuild      # Clean and rebuild"
	@echo "  make test         # Run all executables"

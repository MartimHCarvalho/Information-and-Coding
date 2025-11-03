# Compiler settings
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O3 -g
INCLUDES := -Iincludes
OPENCV_FLAGS := $(shell pkg-config --cflags --libs opencv4 2>/dev/null || echo "")

# Directories
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
OBJ_DIR := $(BUILD_DIR)/obj
DEP_DIR := $(BUILD_DIR)/deps

# Source files - explicitly list what you have
PART1_EX2_SRCS := \
    $(SRC_DIR)/part1/ex2/main.cpp \
    $(SRC_DIR)/part1/ex2/Image.cpp \
    $(SRC_DIR)/part1/ex2/Negative.cpp \
    $(SRC_DIR)/part1/ex2/Mirror.cpp \
    $(SRC_DIR)/part1/ex2/Rotate.cpp \
    $(SRC_DIR)/part1/ex2/Intensity.cpp

CODEC_SRCS := \
    $(SRC_DIR)/codec/losslessImage.cpp

COMMON_SRCS := \
    $(SRC_DIR)/bitstream.cpp \
    $(SRC_DIR)/golomb.cpp

# Object files
PART1_EX2_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(PART1_EX2_SRCS))
CODEC_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(CODEC_SRCS))
COMMON_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(COMMON_SRCS))

# Separate main files from libraries
PART1_MAIN := $(OBJ_DIR)/part1/ex2/main.o
PART1_LIBS := $(filter-out $(PART1_MAIN),$(PART1_EX2_OBJS))

# For codec, if you have a main.cpp
CODEC_MAIN := $(OBJ_DIR)/codec/main.o
CODEC_LIBS := $(filter-out $(CODEC_MAIN),$(CODEC_OBJS))

ALL_OBJECTS := $(PART1_EX2_OBJS) $(CODEC_OBJS) $(COMMON_OBJS)
DEPS := $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$(ALL_OBJECTS))

# Executables
IMAGE_EFFECTS := $(BIN_DIR)/image_effects

# Only add lossless_codec if OpenCV is available AND you have a main
EXECUTABLES := $(IMAGE_EFFECTS)
ifneq ($(OPENCV_FLAGS),)
    ifneq ($(wildcard $(SRC_DIR)/codec/main.cpp),)
        LOSSLESS_CODEC := $(BIN_DIR)/lossless_codec
        EXECUTABLES += $(LOSSLESS_CODEC)
    endif
endif

# Default target
all: directories $(EXECUTABLES)

# Create necessary directories
directories:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)/part1/ex2
	@mkdir -p $(OBJ_DIR)/codec
	@mkdir -p $(DEP_DIR)/part1/ex2
	@mkdir -p $(DEP_DIR)/codec

# Compilation rule for part1 files (no OpenCV)
$(OBJ_DIR)/part1/%.o: $(SRC_DIR)/part1/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(patsubst $(OBJ_DIR)/%,$(DEP_DIR)/%,$@))
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -MF $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@) -c $< -o $@

# Compilation rule for codec files (WITH OpenCV)
$(OBJ_DIR)/codec/%.o: $(SRC_DIR)/codec/%.cpp
	@echo "Compiling $< (with OpenCV)..."
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(patsubst $(OBJ_DIR)/%,$(DEP_DIR)/%,$@))
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPENCV_FLAGS) -MMD -MP -MF $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@) -c $< -o $@

# Compilation rule for common files (no OpenCV)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(patsubst $(OBJ_DIR)/%,$(DEP_DIR)/%,$@))
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -MF $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@) -c $< -o $@

# Link image_effects executable (Part 1 - no OpenCV needed)
$(IMAGE_EFFECTS): $(PART1_MAIN) $(PART1_LIBS)
	@echo "Linking $@..."
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $^ -o $@
	@echo "Build complete: $@"

# Link lossless_codec executable (needs OpenCV and Golomb)
ifneq ($(LOSSLESS_CODEC),)
$(LOSSLESS_CODEC): $(CODEC_MAIN) $(CODEC_LIBS) $(COMMON_OBJS)
	@echo "Linking $@ (with OpenCV)..."
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $^ $(OPENCV_FLAGS) -o $@
	@echo "Build complete: $@"
endif

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
	@echo "=== Source Files ==="
	@echo "PART1_EX2_SRCS:"
	@$(foreach src,$(PART1_EX2_SRCS),echo "  $(src)";)
	@echo ""
	@echo "CODEC_SRCS:"
	@$(foreach src,$(CODEC_SRCS),echo "  $(src)";)
	@echo ""
	@echo "COMMON_SRCS:"
	@$(foreach src,$(COMMON_SRCS),echo "  $(src)";)
	@echo ""
	@echo "=== Object Files ==="
	@echo "PART1_EX2_OBJS:"
	@$(foreach obj,$(PART1_EX2_OBJS),echo "  $(obj)";)
	@echo ""
	@echo "CODEC_OBJS:"
	@$(foreach obj,$(CODEC_OBJS),echo "  $(obj)";)
	@echo ""
	@echo "COMMON_OBJS:"
	@$(foreach obj,$(COMMON_OBJS),echo "  $(obj)";)
	@echo ""
	@echo "=== Configuration ==="
	@echo "OpenCV: $(if $(OPENCV_FLAGS),Found,NOT FOUND)"
	@echo "Executables: $(EXECUTABLES)"

# Show build info
info:
	@echo "Build Configuration:"
	@echo "  CXX:        $(CXX)"
	@echo "  CXXFLAGS:   $(CXXFLAGS)"
	@echo "  INCLUDES:   $(INCLUDES)"
	@echo "  OPENCV:     $(if $(OPENCV_FLAGS),Found,Not Found)"
	@echo ""
	@echo "Executables:"
	@$(foreach exec,$(EXECUTABLES),echo "  $(exec)";)

# Run tests
test: all
	@echo "Testing image_effects..."
	@if [ -f $(IMAGE_EFFECTS) ]; then \
		$(IMAGE_EFFECTS) images/lena.ppm /tmp/test_negative.ppm negative || true; \
		echo "Test output: /tmp/test_negative.ppm"; \
	fi

# Phony targets
.PHONY: all clean directories rebuild debug info test help

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build all targets (default)"
	@echo "  clean        - Remove all build artifacts"
	@echo "  rebuild      - Clean and rebuild everything"
	@echo "  debug        - Show detected source files"
	@echo "  info         - Show build configuration"
	@echo "  test         - Run basic tests"
	@echo "  help         - Show this help message"
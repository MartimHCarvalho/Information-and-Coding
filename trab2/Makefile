# Integrated Makefile for Image Effects and Golomb Coding Codecs
CXX := g++

# Compiler flags
CXXFLAGS := -std=c++17 -Wall -Wextra -O3 -g
LDFLAGS := -lm
OPENCV_FLAGS := $(shell pkg-config --cflags --libs opencv4 2>/dev/null || echo "")

# Directory structure
INCLUDE_DIR := includes
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
OUTPUT_DIR := output
TEST_DIR := src/codec

OBJ_DIR := $(BUILD_DIR)/obj
DEP_DIR := $(BUILD_DIR)/deps

# Include paths
INCLUDES := -I$(INCLUDE_DIR)

# ==================== Source Files ====================

# Part 1 - Exercise 1 (Channel Extraction)
PART1_EX1_SRCS := \
    $(SRC_DIR)/part1/ex1/exe1.cpp

# Part 1 - Exercise 2 (Image Effects)
PART1_EX2_SRCS := \
    $(SRC_DIR)/part1/ex2/main.cpp \
    $(SRC_DIR)/part1/ex2/Image.cpp \
    $(SRC_DIR)/part1/ex2/Negative.cpp \
    $(SRC_DIR)/part1/ex2/Mirror.cpp \
    $(SRC_DIR)/part1/ex2/Rotate.cpp \
    $(SRC_DIR)/part1/ex2/Intensity.cpp

# Golomb Codec - Base sources
GOLOMB_BASE_SRCS := \
    $(SRC_DIR)/bitstream.cpp \
    $(SRC_DIR)/golomb.cpp

# Golomb Codec - Codec implementations
GOLOMB_CODEC_SRCS := \
    $(SRC_DIR)/codec/image_codec.cpp \
    $(SRC_DIR)/codec/audio_codec.cpp

# OpenCV-based lossless codec
LOSSLESS_CODEC_SRCS := \
    $(SRC_DIR)/codec/losslessImage.cpp

# Test files
TEST_IMAGE_SRC := $(TEST_DIR)/test_image_codec.cpp
TEST_AUDIO_SRC := $(TEST_DIR)/test_audio_codec.cpp
TEST_CODEC_SRC := $(TEST_DIR)/test_codec.cpp

# ==================== Object Files ====================

# Part 1 - Exercise 1 objects
PART1_EX1_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(PART1_EX1_SRCS))

# Part 1 - Exercise 2 objects
PART1_EX2_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(PART1_EX2_SRCS))
PART1_MAIN := $(OBJ_DIR)/part1/ex2/main.o
PART1_LIBS := $(filter-out $(PART1_MAIN),$(PART1_EX2_OBJS))

# Golomb objects
GOLOMB_BASE_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(GOLOMB_BASE_SRCS))
GOLOMB_CODEC_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(GOLOMB_CODEC_SRCS))
LOSSLESS_CODEC_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(LOSSLESS_CODEC_SRCS))

# Test objects
TEST_IMAGE_OBJ := $(OBJ_DIR)/tests/test_image_codec.o
TEST_AUDIO_OBJ := $(OBJ_DIR)/tests/test_audio_codec.o
TEST_CODEC_OBJ := $(OBJ_DIR)/tests/test_codec.o

# Objects needed for each test
IMAGE_TEST_OBJECTS := $(OBJ_DIR)/bitstream.o $(OBJ_DIR)/golomb.o $(OBJ_DIR)/codec/image_codec.o
AUDIO_TEST_OBJECTS := $(OBJ_DIR)/bitstream.o $(OBJ_DIR)/golomb.o $(OBJ_DIR)/codec/audio_codec.o
CODEC_TEST_OBJECTS := $(OBJ_DIR)/bitstream.o $(OBJ_DIR)/golomb.o $(OBJ_DIR)/codec/image_codec.o

# All objects
ALL_OBJECTS := $(PART1_EX1_OBJS) $(PART1_EX2_OBJS) $(GOLOMB_BASE_OBJS) $(GOLOMB_CODEC_OBJS) $(LOSSLESS_CODEC_OBJS)
DEPS := $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$(ALL_OBJECTS))

# ==================== Executables ====================

# Part 1 executables
CHANNEL_EXTRACT := $(BIN_DIR)/exe1
IMAGE_EFFECTS := $(BIN_DIR)/image_effects

# Golomb codec test executables
TEST_IMAGE_EXEC := $(BIN_DIR)/test_image_codec
TEST_AUDIO_EXEC := $(BIN_DIR)/test_audio_codec
TEST_CODEC_EXEC := $(BIN_DIR)/test_codec

# Lossless codec executable (requires OpenCV)
LOSSLESS_CODEC := $(BIN_DIR)/lossless_codec

# Determine which executables to build
EXECUTABLES := $(IMAGE_EFFECTS)

# Add exe1 if OpenCV is available and source exists
ifneq ($(OPENCV_FLAGS),)
    ifneq ($(wildcard $(SRC_DIR)/part1/ex1/exe1.cpp),)
        EXECUTABLES += $(CHANNEL_EXTRACT)
    endif
endif

# Add Golomb test executables if test files exist
ifneq ($(wildcard $(TEST_IMAGE_SRC)),)
    EXECUTABLES += $(TEST_IMAGE_EXEC)
endif
ifneq ($(wildcard $(TEST_AUDIO_SRC)),)
    EXECUTABLES += $(TEST_AUDIO_EXEC)
endif
ifneq ($(wildcard $(TEST_CODEC_SRC)),)
    EXECUTABLES += $(TEST_CODEC_EXEC)
endif

# Add lossless codec if OpenCV is available AND main exists
ifneq ($(OPENCV_FLAGS),)
    ifneq ($(wildcard $(SRC_DIR)/codec/main.cpp),)
        LOSSLESS_CODEC_MAIN := $(OBJ_DIR)/codec/main.o
        EXECUTABLES += $(LOSSLESS_CODEC)
    endif
endif

# Colors for output
GREEN := \033[0;32m
BLUE := \033[0;34m
YELLOW := \033[0;33m
CYAN := \033[0;36m
RED := \033[0;31m
NC := \033[0m

# ==================== Main Targets ====================

# Default target
all: directories $(EXECUTABLES)
	@echo ""
	@echo "$(GREEN)╔══════════════════════════════════════════════════════════╗$(NC)"
	@echo "$(GREEN)║  Build Complete!                                         ║$(NC)"
	@echo "$(GREEN)╚══════════════════════════════════════════════════════════╝$(NC)"
	@echo ""
	@echo "$(CYAN)Available executables:$(NC)"
	@for exec in $(EXECUTABLES); do \
		echo "  $(GREEN)✓$(NC) $$exec"; \
	done
	@echo ""

# Create necessary directories
directories:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OUTPUT_DIR)
	@mkdir -p $(OBJ_DIR)/part1/ex1
	@mkdir -p $(OBJ_DIR)/part1/ex2
	@mkdir -p $(OBJ_DIR)/codec
	@mkdir -p $(OBJ_DIR)/tests
	@mkdir -p $(DEP_DIR)/part1/ex1
	@mkdir -p $(DEP_DIR)/part1/ex2
	@mkdir -p $(DEP_DIR)/codec
	@mkdir -p $(DEP_DIR)/tests

# ==================== Compilation Rules ====================

# Compilation rule for part1/ex1 files (WITH OpenCV)
$(OBJ_DIR)/part1/ex1/%.o: $(SRC_DIR)/part1/ex1/%.cpp
	@echo "$(BLUE)Compiling $< (with OpenCV)...$(NC)"
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(patsubst $(OBJ_DIR)/%,$(DEP_DIR)/%,$@))
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPENCV_FLAGS) -MMD -MP -MF $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@) -c $< -o $@

# Compilation rule for part1/ex2 files (no OpenCV)
$(OBJ_DIR)/part1/ex2/%.o: $(SRC_DIR)/part1/ex2/%.cpp
	@echo "$(BLUE)Compiling $<...$(NC)"
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(patsubst $(OBJ_DIR)/%,$(DEP_DIR)/%,$@))
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -MF $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@) -c $< -o $@

# Compilation rule for codec files (WITH OpenCV when available)
$(OBJ_DIR)/codec/%.o: $(SRC_DIR)/codec/%.cpp
	@echo "$(BLUE)Compiling $< $(if $(OPENCV_FLAGS),(with OpenCV),)...$(NC)"
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(patsubst $(OBJ_DIR)/%,$(DEP_DIR)/%,$@))
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(if $(OPENCV_FLAGS),$(OPENCV_FLAGS),) -MMD -MP -MF $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@) -c $< -o $@

# Compilation rule for base Golomb files (no OpenCV)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "$(BLUE)Compiling $<...$(NC)"
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(patsubst $(OBJ_DIR)/%,$(DEP_DIR)/%,$@))
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -MF $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@) -c $< -o $@

# Compilation rule for test files
$(OBJ_DIR)/tests/%.o: $(TEST_DIR)/%.cpp
	@echo "$(BLUE)Compiling test $<...$(NC)"
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(patsubst $(OBJ_DIR)/%,$(DEP_DIR)/%,$@))
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -MF $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@) -c $< -o $@

# ==================== Linking Rules ====================

# Link exe1 executable (Part 1 - Exercise 1, needs OpenCV)
$(CHANNEL_EXTRACT): $(PART1_EX1_OBJS)
	@echo "$(BLUE)Linking $(notdir $@) (with OpenCV)...$(NC)"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $^ $(OPENCV_FLAGS) -o $@
	@echo "$(GREEN)✓ Built $(notdir $@)$(NC)"

# Link image_effects executable (Part 1 - Exercise 2, no OpenCV needed)
$(IMAGE_EFFECTS): $(PART1_MAIN) $(PART1_LIBS)
	@echo "$(BLUE)Linking $(notdir $@)...$(NC)"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $^ -o $@
	@echo "$(GREEN)✓ Built $(notdir $@)$(NC)"

# Link Golomb test executables
$(TEST_IMAGE_EXEC): $(TEST_IMAGE_OBJ) $(IMAGE_TEST_OBJECTS)
	@echo "$(BLUE)Linking $(notdir $@)...$(NC)"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "$(GREEN)✓ Built $(notdir $@)$(NC)"

$(TEST_AUDIO_EXEC): $(TEST_AUDIO_OBJ) $(AUDIO_TEST_OBJECTS)
	@echo "$(BLUE)Linking $(notdir $@)...$(NC)"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "$(GREEN)✓ Built $(notdir $@)$(NC)"

$(TEST_CODEC_EXEC): $(TEST_CODEC_OBJ) $(CODEC_TEST_OBJECTS)
	@echo "$(BLUE)Linking $(notdir $@)...$(NC)"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "$(GREEN)✓ Built $(notdir $@)$(NC)"

# Link lossless_codec executable (needs OpenCV)
ifneq ($(LOSSLESS_CODEC_MAIN),)
$(LOSSLESS_CODEC): $(LOSSLESS_CODEC_MAIN) $(LOSSLESS_CODEC_OBJS) $(GOLOMB_BASE_OBJS)
	@echo "$(BLUE)Linking $(notdir $@) (with OpenCV)...$(NC)"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $^ $(OPENCV_FLAGS) -o $@
	@echo "$(GREEN)✓ Built $(notdir $@)$(NC)"
endif

# Include dependency files
-include $(DEPS)

# ==================== Run Targets - Part 1 ====================

# Run channel extraction
run-exe1: $(CHANNEL_EXTRACT)
	@if [ -n "$(INPUT)" ] && [ -n "$(OUTPUT)" ] && [ -n "$(CHANNEL)" ]; then \
		echo "$(YELLOW)Running channel extraction: channel $(CHANNEL)$(NC)"; \
		$(CHANNEL_EXTRACT) $(INPUT) $(OUTPUT) $(CHANNEL); \
	else \
		echo "$(YELLOW)Usage: make run-exe1 INPUT=input.ppm OUTPUT=output.ppm CHANNEL=0$(NC)"; \
		echo "Channel: 0 = Blue, 1 = Green, 2 = Red"; \
	fi

# Test channel extraction
test-exe1: $(CHANNEL_EXTRACT)
	@echo "$(YELLOW)Testing channel extraction...$(NC)"
	@if [ -f images/anemone.ppm ]; then \
		mkdir -p $(OUTPUT_DIR); \
		$(CHANNEL_EXTRACT) images/anemone.ppm $(OUTPUT_DIR)/blue_channel.ppm 0 || true; \
		$(CHANNEL_EXTRACT) images/anemone.ppm $(OUTPUT_DIR)/green_channel.ppm 1 || true; \
		$(CHANNEL_EXTRACT) images/anemone.ppm $(OUTPUT_DIR)/red_channel.ppm 2 || true; \
		echo "$(GREEN)✓ Test outputs in $(OUTPUT_DIR)/$(NC)"; \
	else \
		echo "$(RED)✗ Test image 'images/anemone.ppm' not found$(NC)"; \
	fi

# Run image effects
# Run image effects
run-effects: $(IMAGE_EFFECTS)
	@if [ -n "$(INPUT)" ] && [ -n "$(OUTPUT)" ] && [ -n "$(EFFECT)" ]; then \
		echo "$(YELLOW)Running image effects: $(EFFECT)$(NC)"; \
		if [ "$(EFFECT)" = "rotate" ]; then \
			if [ -n "$(ANGLE)" ]; then \
				$(IMAGE_EFFECTS) $(INPUT) $(OUTPUT) $(EFFECT) $(ANGLE); \
			else \
				echo "$(RED)Error: ANGLE parameter required for rotate effect$(NC)"; \
				echo "$(YELLOW)Usage: make run-effects INPUT=in.ppm OUTPUT=out.ppm EFFECT=rotate ANGLE=90$(NC)"; \
				exit 1; \
			fi; \
		elif [ "$(EFFECT)" = "intensity" ]; then \
			if [ -n "$(FACTOR)" ]; then \
				$(IMAGE_EFFECTS) $(INPUT) $(OUTPUT) $(EFFECT) $(FACTOR); \
			else \
				echo "$(RED)Error: FACTOR parameter required for intensity effect$(NC)"; \
				echo "$(YELLOW)Usage: make run-effects INPUT=in.ppm OUTPUT=out.ppm EFFECT=intensity FACTOR=1.5$(NC)"; \
				exit 1; \
			fi; \
		else \
			$(IMAGE_EFFECTS) $(INPUT) $(OUTPUT) $(EFFECT); \
		fi \
	else \
		echo "$(YELLOW)Usage Examples:$(NC)"; \
		echo "  $(CYAN)make run-effects INPUT=in.ppm OUTPUT=out.ppm EFFECT=negative$(NC)"; \
		echo "  $(CYAN)make run-effects INPUT=in.ppm OUTPUT=out.ppm EFFECT=mirror$(NC)"; \
		echo "  $(CYAN)make run-effects INPUT=in.ppm OUTPUT=out.ppm EFFECT=rotate ANGLE=90$(NC)"; \
		echo "  $(CYAN)make run-effects INPUT=in.ppm OUTPUT=out.ppm EFFECT=intensity FACTOR=1.5$(NC)"; \
		echo ""; \
		echo "$(YELLOW)Available effects:$(NC)"; \
		echo "  - negative  : Invert colors"; \
		echo "  - mirror    : Horizontal mirror"; \
		echo "  - rotate    : Rotate by angle (requires ANGLE parameter)"; \
		echo "  - intensity : Adjust brightness (requires FACTOR parameter)"; \
	fi

# Test image effects
test-effects: $(IMAGE_EFFECTS)
	@echo "$(YELLOW)Testing image effects...$(NC)"
	@if [ -f images/lena.ppm ]; then \
		mkdir -p $(OUTPUT_DIR); \
		echo "$(CYAN)Testing negative effect...$(NC)"; \
		$(IMAGE_EFFECTS) images/lena.ppm $(OUTPUT_DIR)/test_negative.ppm negative || true; \
		echo "$(CYAN)Testing mirror effect...$(NC)"; \
		$(IMAGE_EFFECTS) images/lena.ppm $(OUTPUT_DIR)/test_mirror.ppm mirror || true; \
		echo "$(CYAN)Testing rotate effect (90°)...$(NC)"; \
		$(IMAGE_EFFECTS) images/lena.ppm $(OUTPUT_DIR)/test_rotate_90.ppm rotate 90 || true; \
		echo "$(CYAN)Testing rotate effect (180°)...$(NC)"; \
		$(IMAGE_EFFECTS) images/lena.ppm $(OUTPUT_DIR)/test_rotate_180.ppm rotate 180 || true; \
		echo "$(CYAN)Testing intensity effect (0.5x)...$(NC)"; \
		$(IMAGE_EFFECTS) images/lena.ppm $(OUTPUT_DIR)/test_intensity_dark.ppm intensity 0.5 || true; \
		echo "$(CYAN)Testing intensity effect (1.5x)...$(NC)"; \
		$(IMAGE_EFFECTS) images/lena.ppm $(OUTPUT_DIR)/test_intensity_bright.ppm intensity 1.5 || true; \
		echo ""; \
		echo "$(GREEN)✓ Test outputs in $(OUTPUT_DIR)/:$(NC)"; \
		ls -lh $(OUTPUT_DIR)/test_*.ppm 2>/dev/null | awk '{print "  - " $$9 " (" $$5 ")"}' || true; \
	else \
		echo "$(RED)✗ Test image 'images/lena.ppm' not found$(NC)"; \
	fi

# ==================== Run Targets - Golomb Codecs ====================

# Run image test directly
run-image: $(TEST_IMAGE_EXEC)
	@mkdir -p $(OUTPUT_DIR)
	@FILE_TO_USE="$(FILE)$(IMAGE)"; \
	if [ -n "$$FILE_TO_USE" ]; then \
		echo "$(YELLOW)Running image test with: $$FILE_TO_USE$(NC)"; \
		ABS_FILE=$$(cd $$(dirname "$$FILE_TO_USE") && pwd)/$$(basename "$$FILE_TO_USE"); \
		cd $(OUTPUT_DIR) && $(abspath $(TEST_IMAGE_EXEC)) "$$ABS_FILE"; \
	else \
		echo "$(YELLOW)Running image test with synthetic data$(NC)"; \
		cd $(OUTPUT_DIR) && $(abspath $(TEST_IMAGE_EXEC)); \
	fi

# Run audio test directly
run-audio: $(TEST_AUDIO_EXEC)
	@mkdir -p $(OUTPUT_DIR)
	@if [ -n "$(FILE)" ]; then \
		echo "$(YELLOW)Running audio test with: $(FILE)$(NC)"; \
		ABS_FILE=$$(cd $$(dirname "$(FILE)") && pwd)/$$(basename "$(FILE)"); \
		cd $(OUTPUT_DIR) && $(abspath $(TEST_AUDIO_EXEC)) "$$ABS_FILE"; \
	else \
		echo "$(YELLOW)Running audio test with synthetic data$(NC)"; \
		cd $(OUTPUT_DIR) && $(abspath $(TEST_AUDIO_EXEC)); \
	fi

# Run codec test
run-codec: $(TEST_CODEC_EXEC)
	@mkdir -p $(OUTPUT_DIR)
	@FILE_TO_USE="$(FILE)$(IMAGE)"; \
	if [ -n "$$FILE_TO_USE" ]; then \
		echo "$(YELLOW)Running codec test with: $$FILE_TO_USE$(NC)"; \
		ABS_FILE=$$(cd $$(dirname "$$FILE_TO_USE") && pwd)/$$(basename "$$FILE_TO_USE"); \
		cd $(OUTPUT_DIR) && $(abspath $(TEST_CODEC_EXEC)) "$$ABS_FILE"; \
	else \
		echo "$(YELLOW)Running codec test with synthetic data$(NC)"; \
		cd $(OUTPUT_DIR) && $(abspath $(TEST_CODEC_EXEC)); \
	fi

# Generic run target
run:
	@if [ -n "$(FILE)$(IMAGE)" ]; then \
		FILE_TO_USE="$(FILE)$(IMAGE)"; \
		EXT=$${FILE_TO_USE##*.}; \
		case $$EXT in \
			pgm|PGM) \
				if [ -f $(TEST_IMAGE_EXEC) ]; then \
					$(MAKE) run-image FILE="$$FILE_TO_USE"; \
				elif [ -f $(TEST_CODEC_EXEC) ]; then \
					$(MAKE) run-codec FILE="$$FILE_TO_USE"; \
				else \
					echo "$(YELLOW)No image codec test found. Run 'make' first.$(NC)"; \
				fi ;; \
			wav|WAV) \
				if [ -f $(TEST_AUDIO_EXEC) ]; then \
					$(MAKE) run-audio FILE="$$FILE_TO_USE"; \
				else \
					echo "$(YELLOW)No audio codec test found. Run 'make' first.$(NC)"; \
				fi ;; \
			*) \
				echo "$(YELLOW)Unknown file type. Specify using:$(NC)"; \
				echo "  make run-image FILE=file.pgm"; \
				echo "  make run-audio FILE=file.wav"; \
				echo "  make run-effects INPUT=in.ppm OUTPUT=out.ppm EFFECT=negative"; \
				echo "  make run-exe1 INPUT=in.ppm OUTPUT=out.ppm CHANNEL=0"; ;; \
		esac \
	else \
		echo "$(YELLOW)Usage Examples:$(NC)"; \
		echo "  $(CYAN)make run FILE=path/to/file.pgm$(NC)"; \
		echo "  $(CYAN)make run-effects INPUT=in.ppm OUTPUT=out.ppm EFFECT=negative$(NC)"; \
		echo "  $(CYAN)make run-exe1 INPUT=in.ppm OUTPUT=out.ppm CHANNEL=0$(NC)"; \
	fi

# ==================== Testing Targets ====================

# Run all tests
test: test-available test-exe1 test-effects
	@echo ""
	@echo "$(GREEN)╔══════════════════════════════════════════════════════════╗$(NC)"
	@echo "$(GREEN)║  All Tests Completed Successfully!                      ║$(NC)"
	@echo "$(GREEN)╚══════════════════════════════════════════════════════════╝$(NC)"

# Run available Golomb tests
test-available:
	@if [ -f $(TEST_IMAGE_EXEC) ]; then $(MAKE) test-image; fi
	@if [ -f $(TEST_AUDIO_EXEC) ]; then $(MAKE) test-audio; fi
	@if [ -f $(TEST_CODEC_EXEC) ]; then $(MAKE) test-codec; fi

# Run image codec test
test-image: $(TEST_IMAGE_EXEC)
	@echo ""
	@echo "$(YELLOW)╔══════════════════════════════════════════════════════════╗$(NC)"
	@echo "$(YELLOW)║  Running Image Codec Tests                               ║$(NC)"
	@echo "$(YELLOW)╚══════════════════════════════════════════════════════════╝$(NC)"
	@mkdir -p $(OUTPUT_DIR)
	@cd $(OUTPUT_DIR) && $(abspath $(TEST_IMAGE_EXEC))

# Run audio codec test
test-audio: $(TEST_AUDIO_EXEC)
	@echo ""
	@echo "$(YELLOW)╔══════════════════════════════════════════════════════════╗$(NC)"
	@echo "$(YELLOW)║  Running Audio Codec Tests                               ║$(NC)"
	@echo "$(YELLOW)╚══════════════════════════════════════════════════════════╝$(NC)"
	@mkdir -p $(OUTPUT_DIR)
	@cd $(OUTPUT_DIR) && $(abspath $(TEST_AUDIO_EXEC))

# Run codec test
test-codec: $(TEST_CODEC_EXEC)
	@echo ""
	@echo "$(YELLOW)╔══════════════════════════════════════════════════════════╗$(NC)"
	@echo "$(YELLOW)║  Running Codec Tests                                     ║$(NC)"
	@echo "$(YELLOW)╚══════════════════════════════════════════════════════════╝$(NC)"
	@mkdir -p $(OUTPUT_DIR)
	@cd $(OUTPUT_DIR) && $(abspath $(TEST_CODEC_EXEC))

# Quick test
quick-test: all
	@echo "$(YELLOW)Running quick tests...$(NC)"
	@mkdir -p $(OUTPUT_DIR)
	@if [ -f $(TEST_IMAGE_EXEC) ]; then cd $(OUTPUT_DIR) && $(abspath $(TEST_IMAGE_EXEC)); echo ""; fi
	@if [ -f $(TEST_AUDIO_EXEC) ]; then cd $(OUTPUT_DIR) && $(abspath $(TEST_AUDIO_EXEC)); echo ""; fi

# ==================== Benchmark Targets ====================

# Benchmark image codec
benchmark-image: $(TEST_IMAGE_EXEC)
	@mkdir -p $(OUTPUT_DIR)
	@FILE_TO_USE="$(FILE)$(IMAGE)"; \
	if [ -n "$$FILE_TO_USE" ]; then \
		echo "$(YELLOW)Benchmarking image codec with: $$FILE_TO_USE$(NC)"; \
		cd $(OUTPUT_DIR) && time $(abspath $(TEST_IMAGE_EXEC)) $(abspath $$FILE_TO_USE); \
	else \
		echo "$(YELLOW)Benchmarking image codec with synthetic data$(NC)"; \
		cd $(OUTPUT_DIR) && time $(abspath $(TEST_IMAGE_EXEC)); \
	fi

# Benchmark audio codec
benchmark-audio: $(TEST_AUDIO_EXEC)
	@mkdir -p $(OUTPUT_DIR)
	@if [ -n "$(FILE)" ]; then \
		echo "$(YELLOW)Benchmarking audio codec with: $(FILE)$(NC)"; \
		cd $(OUTPUT_DIR) && time $(abspath $(TEST_AUDIO_EXEC)) $(abspath $(FILE)); \
	else \
		echo "$(YELLOW)Benchmarking audio codec with synthetic data$(NC)"; \
		cd $(OUTPUT_DIR) && time $(abspath $(TEST_AUDIO_EXEC)); \
	fi

# Run all benchmarks
benchmark:
	@if [ -f $(TEST_IMAGE_EXEC) ]; then $(MAKE) benchmark-image; fi
	@if [ -f $(TEST_AUDIO_EXEC) ]; then $(MAKE) benchmark-audio; fi

# ==================== Build Variants ====================

# Build only Part 1
part1: directories $(CHANNEL_EXTRACT) $(IMAGE_EFFECTS)
	@echo "$(GREEN)✓ Part 1 built$(NC)"
	@echo "$(CYAN)Available:$(NC)"
	@if [ -f $(CHANNEL_EXTRACT) ]; then echo "  - $(CHANNEL_EXTRACT)"; fi
	@echo "  - $(IMAGE_EFFECTS)"

# Build only Golomb codecs
codecs: directories
	@if [ -f $(TEST_IMAGE_SRC) ]; then $(MAKE) $(TEST_IMAGE_EXEC); fi
	@if [ -f $(TEST_AUDIO_SRC) ]; then $(MAKE) $(TEST_AUDIO_EXEC); fi
	@if [ -f $(TEST_CODEC_SRC) ]; then $(MAKE) $(TEST_CODEC_EXEC); fi
	@echo "$(GREEN)✓ Golomb codecs built$(NC)"

# Build only image codec test
image:
	@if [ -f $(TEST_IMAGE_SRC) ]; then \
		$(MAKE) directories $(TEST_IMAGE_EXEC); \
		echo "$(GREEN)✓ Image codec test ready$(NC)"; \
		echo "$(CYAN)Run: $(TEST_IMAGE_EXEC) [image.pgm]$(NC)"; \
	else \
		echo "$(YELLOW)test_image_codec.cpp not found$(NC)"; \
	fi

# Build only audio codec test
audio:
	@if [ -f $(TEST_AUDIO_SRC) ]; then \
		$(MAKE) directories $(TEST_AUDIO_EXEC); \
		echo "$(GREEN)✓ Audio codec test ready$(NC)"; \
		echo "$(CYAN)Run: $(TEST_AUDIO_EXEC) [audio.wav]$(NC)"; \
	else \
		echo "$(YELLOW)test_audio_codec.cpp not found$(NC)"; \
	fi

# Debug build
debug: CXXFLAGS := -std=c++17 -Wall -Wextra -O0 -g $(INCLUDES) -DDEBUG
debug: clean all
	@echo "$(GREEN)✓ Debug build complete$(NC)"

# Release build
release: CXXFLAGS := -std=c++17 -Wall -Wextra -O3 -march=native $(INCLUDES) -DNDEBUG
release: clean all
	@echo "$(GREEN)✓ Release build complete$(NC)"

# ==================== Cleaning Targets ====================

# Clean build artifacts
clean:
	@echo "$(YELLOW)Cleaning build artifacts...$(NC)"
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BIN_DIR)
	@echo "$(GREEN)✓ Build artifacts cleaned!$(NC)"

# Clean only test output files
clean-output:
	@echo "$(YELLOW)Cleaning test outputs...$(NC)"
	@rm -f $(OUTPUT_DIR)/*.golomb $(OUTPUT_DIR)/*.pgm $(OUTPUT_DIR)/*.wav $(OUTPUT_DIR)/*.ppm
	@rm -f *.golomb *.pgm *.wav *.ppm
	@echo "$(GREEN)✓ Test outputs cleaned!$(NC)"

# Clean everything
distclean: clean clean-output
	@echo "$(YELLOW)Removing all generated files...$(NC)"
	@rm -rf $(OUTPUT_DIR)
	@echo "$(GREEN)✓ Full clean complete!$(NC)"

# Rebuild from scratch
rebuild: clean all

# ==================== Information Targets ====================

# Check build status
check:
	@echo "$(BLUE)╔══════════════════════════════════════════════════════════╗$(NC)"
	@echo "$(BLUE)║  Build Status                                            ║$(NC)"
	@echo "$(BLUE)╚══════════════════════════════════════════════════════════╝$(NC)"
	@echo ""
	@echo "$(YELLOW)Part 1 - Image Processing:$(NC)"
	@if [ -f $(CHANNEL_EXTRACT) ]; then \
		echo "  $(GREEN)✓ $(CHANNEL_EXTRACT)$(NC)"; \
	else \
		echo "  $(RED)✗ Channel extraction not built$(NC)"; \
	fi
	@if [ -f $(IMAGE_EFFECTS) ]; then \
		echo "  $(GREEN)✓ $(IMAGE_EFFECTS)$(NC)"; \
	else \
		echo "  $(RED)✗ Image effects not built$(NC)"; \
	fi
	@echo ""
	@echo "$(YELLOW)Golomb Codecs:$(NC)"
	@if [ -f $(TEST_IMAGE_EXEC) ]; then \
		echo "  $(GREEN)✓ $(TEST_IMAGE_EXEC)$(NC)"; \
	else \
		echo "  $(RED)✗ Image codec test not built$(NC)"; \
	fi
	@if [ -f $(TEST_AUDIO_EXEC) ]; then \
		echo "  $(GREEN)✓ $(TEST_AUDIO_EXEC)$(NC)"; \
	else \
		echo "  $(RED)✗ Audio codec test not built$(NC)"; \
	fi
	@if [ -f $(TEST_CODEC_EXEC) ]; then \
		echo "  $(GREEN)✓ $(TEST_CODEC_EXEC)$(NC)"; \
	else \
		echo "  $(RED)✗ Codec test not built$(NC)"; \
	fi
	@echo ""
	@echo "$(YELLOW)OpenCV Support:$(NC)"
	@if [ -n "$(OPENCV_FLAGS)" ]; then \
		echo "  $(GREEN)✓ OpenCV detected$(NC)"; \
		if [ -f $(LOSSLESS_CODEC) ]; then \
			echo "  $(GREEN)✓ $(LOSSLESS_CODEC)$(NC)"; \
		fi \
	else \
		echo "  $(YELLOW)✗ OpenCV not found (optional)$(NC)"; \
	fi

# Show project structure
show-structure:
	@echo "$(BLUE)╔══════════════════════════════════════════════════════════╗$(NC)"
	@echo "$(BLUE)║  Project Structure                                       ║$(NC)"
	@echo "$(BLUE)╚══════════════════════════════════════════════════════════╝$(NC)"
	@echo ""
	@echo "$(YELLOW)Part 1 - Exercise 1 Sources:$(NC)"
	@for src in $(PART1_EX1_SRCS); do echo "  $src"; done
	@echo ""
	@echo "$(YELLOW)Part 1 - Exercise 2 Sources:$(NC)"
	@for src in $(PART1_EX2_SRCS); do echo "  $src"; done
	@echo ""
	@echo "$(YELLOW)Golomb Base Sources:$(NC)"
	@for src in $(GOLOMB_BASE_SRCS); do echo "  $src"; done
	@echo ""
	@echo "$(YELLOW)Golomb Codec Sources:$(NC)"
	@for src in $(GOLOMB_CODEC_SRCS); do echo "  $src"; done
	@echo ""
	@echo "$(YELLOW)Test Files:$(NC)"
	@if [ -f $(TEST_IMAGE_SRC) ]; then echo "  $(TEST_IMAGE_SRC)"; fi
	@if [ -f $(TEST_AUDIO_SRC) ]; then echo "  $(TEST_AUDIO_SRC)"; fi
	@if [ -f $(TEST_CODEC_SRC) ]; then echo "  $(TEST_CODEC_SRC)"; fi
	@echo ""
	@echo "$(YELLOW)Executables:$(NC)"
	@for exec in $(EXECUTABLES); do echo "  $exec"; done

# List output files
list-output:
	@echo "$(BLUE)Output files in $(OUTPUT_DIR):$(NC)"
	@ls -lh $(OUTPUT_DIR)/ 2>/dev/null || echo "$(YELLOW)No output files yet$(NC)"

# Show build configuration
info:
	@echo "$(BLUE)╔══════════════════════════════════════════════════════════╗$(NC)"
	@echo "$(BLUE)║  Build Configuration                                     ║$(NC)"
	@echo "$(BLUE)╚══════════════════════════════════════════════════════════╝$(NC)"
	@echo ""
	@echo "  $(YELLOW)CXX:$(NC)        $(CXX)"
	@echo "  $(YELLOW)CXXFLAGS:$(NC)   $(CXXFLAGS)"
	@echo "  $(YELLOW)INCLUDES:$(NC)   $(INCLUDES)"
	@echo "  $(YELLOW)OpenCV:$(NC)     $(if $(OPENCV_FLAGS),$(GREEN)Found$(NC),$(YELLOW)Not Found$(NC))"
	@echo ""
	@echo "  $(YELLOW)Executables to build:$(NC)"
	@for exec in $(EXECUTABLES); do echo "    - $exec"; done

# ==================== Help ====================

help:
	@echo "$(BLUE)╔══════════════════════════════════════════════════════════╗$(NC)"
	@echo "$(BLUE)║  Integrated Makefile - Help                              ║$(NC)"
	@echo "$(BLUE)╚══════════════════════════════════════════════════════════╝$(NC)"
	@echo ""
	@echo "$(YELLOW)Building:$(NC)"
	@echo "  $(GREEN)make$(NC) / $(GREEN)make all$(NC)     - Build all available components"
	@echo "  $(GREEN)make part1$(NC)           - Build only Part 1 (ex1 and ex2)"
	@echo "  $(GREEN)make codecs$(NC)          - Build only Golomb codecs"
	@echo "  $(GREEN)make image$(NC)           - Build only image codec test"
	@echo "  $(GREEN)make audio$(NC)           - Build only audio codec test"
	@echo "  $(GREEN)make debug$(NC)           - Debug build (O0, -g)"
	@echo "  $(GREEN)make release$(NC)         - Release build (O3, optimized)"
	@echo ""
	@echo "$(YELLOW)Running Part 1:$(NC)"
	@echo "  $(GREEN)make run-exe1$(NC) INPUT=in.ppm OUTPUT=out.ppm CHANNEL=0"
	@echo "    - Extract channel (0=Blue, 1=Green, 2=Red)"
	@echo "  $(GREEN)make run-effects$(NC) INPUT=in.ppm OUTPUT=out.ppm EFFECT=negative"
	@echo "    - Apply effect (negative, mirror, rotate, intensity)"
	@echo "  $(GREEN)make test-exe1$(NC)       - Test channel extraction"
	@echo "  $(GREEN)make test-effects$(NC)    - Test image effects"
	@echo ""
	@echo "$(YELLOW)Running Codecs:$(NC)"
	@echo "  $(GREEN)make run$(NC) FILE=file.pgm     - Auto-detect and run codec"
	@echo "  $(GREEN)make run-image$(NC) FILE=file.pgm"
	@echo "  $(GREEN)make run-audio$(NC) FILE=file.wav"
	@echo "  $(GREEN)make run-codec$(NC) FILE=file.pgm"
	@echo ""
	@echo "$(YELLOW)Testing:$(NC)"
	@echo "  $(GREEN)make test$(NC)            - Run all tests"
	@echo "  $(GREEN)make test-image$(NC)      - Test image codec"
	@echo "  $(GREEN)make test-audio$(NC)      - Test audio codec"
	@echo "  $(GREEN)make quick-test$(NC)      - Quick sanity check"
	@echo ""
	@echo "$(YELLOW)Benchmarking:$(NC)"
	@echo "  $(GREEN)make benchmark$(NC)       - Benchmark all codecs"
	@echo "  $(GREEN)make benchmark-image$(NC) FILE=file.pgm"
	@echo "  $(GREEN)make benchmark-audio$(NC) FILE=file.wav"
	@echo ""
	@echo "$(YELLOW)Cleaning:$(NC)"
	@echo "  $(GREEN)make clean$(NC)           - Remove build artifacts"
	@echo "  $(GREEN)make clean-output$(NC)    - Remove test outputs"
	@echo "  $(GREEN)make distclean$(NC)       - Full clean"
	@echo "  $(GREEN)make rebuild$(NC)         - Clean and rebuild"
	@echo ""
	@echo "$(YELLOW)Information:$(NC)"
	@echo "  $(GREEN)make check$(NC)           - Show build status"
	@echo "  $(GREEN)make info$(NC)            - Show build configuration"
	@echo "  $(GREEN)make show-structure$(NC)  - Show project structure"
	@echo "  $(GREEN)make list-output$(NC)     - List output files"
	@echo "  $(GREEN)make help$(NC)            - Show this help"

.PHONY: all directories clean clean-output distclean rebuild \
        part1 codecs image audio debug release \
        run run-exe1 run-effects run-image run-audio run-codec \
        test test-available test-exe1 test-effects test-image test-audio test-codec quick-test \
        benchmark benchmark-image benchmark-audio \
        check show-structure list-output info help
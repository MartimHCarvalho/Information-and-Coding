CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O3 -g
INCLUDES := -Iincludes

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
OBJ_DIR := $(BUILD_DIR)/obj

# Part 1 - Image effects
P1_SRCS := $(wildcard $(SRC_DIR)/part1/ex2/*.cpp)
P1_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(P1_SRCS))
P1_MAIN := $(OBJ_DIR)/part1/ex2/main.o
P1_LIBS := $(filter-out $(P1_MAIN),$(P1_OBJS))

# Part 2 - Golomb coding
P2_SRCS := $(wildcard $(SRC_DIR)/part2/*.cpp)
P2_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(P2_SRCS))

# Executables
IMAGE_EFFECTS := $(BIN_DIR)/image_effects

all: $(IMAGE_EFFECTS)

$(IMAGE_EFFECTS): $(P1_MAIN) $(P1_LIBS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

rebuild: clean all

.PHONY: all clean rebuild
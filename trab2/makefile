CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O3 -Iincludes
LDFLAGS =

# Directories
SRC_DIR = src
INC_DIR = includes
OBJ_DIR = obj
BIN_DIR = bin

# Source files
SOURCES = $(SRC_DIR)/golomb.cpp \
          $(SRC_DIR)/bitstream.cpp \
          image_codec.cpp \
          $(SRC_DIR)/test_codec.cpp

# Object files
OBJECTS = $(OBJ_DIR)/golomb.o \
          $(OBJ_DIR)/bitstream.o \
          $(OBJ_DIR)/image_codec.o \
          $(OBJ_DIR)/test_codec.o

# Target executable
TARGET = $(BIN_DIR)/test_codec

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Link
$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^
	@echo "Build complete: $(TARGET)"

# Compile source files
$(OBJ_DIR)/golomb.o: ../$(SRC_DIR)/golomb.cpp ../$(INC_DIR)/golomb.hpp ../$(INC_DIR)/bitstream.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/bitstream.o: ../$(SRC_DIR)/bitstream.cpp ../$(INC_DIR)/bitstream.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/image_codec.o: ./image_codec.cpp ./image_codec.hpp ../$(INC_DIR)/golomb.hpp ../$(INC_DIR)/bitstream.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/test_codec.o: test_codec.cpp image_codec.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	rm -f *.golomb *.pgm
	@echo "Clean complete"

# Run the test
test: all
	./$(TARGET)

# Run with a specific PGM file
run: all
	@if [ -z "$(IMAGE)" ]; then \
		echo "Usage: make run IMAGE=path/to/image.pgm"; \
		./$(TARGET); \
	else \
		./$(TARGET) $(IMAGE); \
	fi

.PHONY: all directories clean test run
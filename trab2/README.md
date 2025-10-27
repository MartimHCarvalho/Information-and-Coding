# Part-I

Image compression and Golomb coding implementation.

## Building

### Using Make (recommended)
```bash
# Build everything
make

# Clean build artifacts
make clean

# Rebuild from scratch
make rebuild

# Show build configuration
make info

# Get help
make help
```

### Using CMake (alternative)
```bash
# Create build directory
mkdir -p build && cd build

# Configure and build
cmake ..
make

# Return to project root
cd ..
```

## Directory Structure
```
trab2/
├── src/           # Source files
├── includes/      # Header files
├── images/        # Test images
├── build/         # Build artifacts (generated)
│   ├── obj/       # Object files
│   └── deps/      # Dependency files
└── bin/           # Executables (generated)
```

## Running

### Image Codec
```bash
./bin/image_codec images/lena.ppm output.enc
```

### Golomb Test
```bash
./bin/golomb_test
```

## Exercise 1

```bash
./bin/image_codec encode <input.ppm> <output.enc>
./bin/image_codec decode <input.enc> <output.ppm>
```

## Exercise 2

```bash
./bin/golomb_test <m_parameter>
```

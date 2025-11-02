# Information and Coding - Assignment 2
## Building

```bash
make           # Build all executables
make clean     # Remove build artifacts
make rebuild   # Clean and rebuild
```

## Directory Structure
```
trab2/
├── src/
│   ├── part1/
│   │   ├── ex1/       # Exercise 1
│   │   └── ex2/       # Image effects (ex2)
│   └── part2/         # Golomb coding library
├── includes/          # Header files
├── images/            # Test images (PPM format)
├── build/             # Build artifacts (generated)
└── bin/               # Executables (generated)
```

## Part 1 - Image Effects

The `image_effects` program supports various image transformations on PPM files.

### Usage

```bash
./bin/image_effects <input_file> <output_file> <operation> [parameters]
```

### Supported Operations

| Operation  | Description                    | Example                                                    |
|------------|--------------------------------|------------------------------------------------------------|
| negative   | Creates the image negative     | `./bin/image_effects input.ppm output.ppm negative`  |
| mirror_h   | Horizontal mirroring           | `./bin/image_effects input.ppm output.ppm mirror_h`  |
| mirror_v   | Vertical mirroring             | `./bin/image_effects input.ppm output.ppm mirror_v`  |
| rotate     | Rotate by multiples of 90°     | `./bin/image_effects input.ppm output.ppm rotate 2`  |
| intensity  | Adjust brightness (+ or - value) | `./bin/image_effects input.ppm output.ppm intensity 40` |

## Part 2 - Golomb Coding

Library for Golomb encoding/decoding (bitstream and golomb modules).

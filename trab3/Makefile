# SafeTensors Multi-Algorithm Compressor
# Supports: ZSTD, LZ4, DEFLATE, LZMA

.PHONY: all build clean test test-all benchmark compare help install deps

# Default target
all: build

# Build the project
build:
	@echo "Building SafeTensors Compressor..."
	@mkdir -p build bin output
	@cd build && cmake .. && make -j$(shell nproc 2>/dev/null || echo 4)
	@echo ""
	@echo "✓ Build complete! Executable: ./bin/compressor"
	@echo ""

# Clean all build artifacts
clean:
	@rm -rf build bin
	@rm -f output/*.json output/*.csv output/*.stcmp output/*.safetensors
	@echo "✓ Clean complete!"

# Quick integrity test with ZSTD (default algorithm)
test: build
	@echo "=== Running Quick Compression Test (ZSTD) ==="
	@./bin/compressor compress test/model.safetensors output/test.stcmp zstd balanced
	@echo ""
	@echo "=== Testing Decompression ==="
	@./bin/compressor decompress output/test.stcmp output/restored.safetensors
	@echo ""
	@echo "=== Verifying Integrity ==="
	@if cmp -s test/model.safetensors output/restored.safetensors; then \
		echo "✓ SUCCESS: Files match perfectly!"; \
	else \
		echo "✗ ERROR: Files do not match!"; \
		exit 1; \
	fi
	@rm -f output/test.stcmp output/restored.safetensors
	@echo ""

# Test all algorithms and modes
test-all: build
	@echo "=== Testing All Algorithms ==="
	@echo ""
	@for algo in lz4 deflate zstd lzma; do \
		for mode in fast balanced maximum; do \
			echo "Testing $$algo ($$mode)..."; \
			./bin/compressor compress test/model.safetensors output/test_$$algo\_$$mode.stcmp $$algo $$mode || exit 1; \
			./bin/compressor decompress output/test_$$algo\_$$mode.stcmp output/restored_$$algo\_$$mode.safetensors || exit 1; \
			if cmp -s test/model.safetensors output/restored_$$algo\_$$mode.safetensors; then \
				echo "  ✓ $$algo ($$mode) passed"; \
			else \
				echo "  ✗ $$algo ($$mode) FAILED"; \
				exit 1; \
			fi; \
			rm -f output/test_$$algo\_$$mode.stcmp output/restored_$$algo\_$$mode.safetensors; \
		done; \
		echo ""; \
	done
	@echo "✓ All algorithm tests passed!"
	@echo ""

# Run comprehensive benchmark (all algorithms, all modes)
benchmark: build
	@echo "=== Running Comprehensive Benchmark ==="
	@./bin/compressor benchmark test/model.safetensors
	@echo ""
	@echo "Results saved to:"
	@echo "  - output/benchmark_results.json"
	@echo "  - output/benchmark_results.csv"
	@echo ""

# Compare algorithms at specific mode (default: balanced)
compare: build
	@echo "=== Algorithm Comparison (Balanced Mode) ==="
	@./bin/compressor compare test/model.safetensors balanced
	@echo ""
	@echo "Results saved to:"
	@echo "  - output/comparison_results.json"
	@echo "  - output/comparison_results.csv"
	@echo ""

# Compare at fast mode
compare-fast: build
	@./bin/compressor compare test/model.safetensors fast

# Compare at maximum mode
compare-max: build
	@./bin/compressor compare test/model.safetensors maximum

# Install dependencies (Ubuntu/Debian)
deps:
	@echo "Installing dependencies..."
	@sudo apt-get update
	@sudo apt-get install -y \
		build-essential \
		cmake \
		libzstd-dev \
		liblz4-dev \
		zlib1g-dev \
		liblzma-dev
	@echo "✓ Dependencies installed!"

# Install the binary system-wide
install: build
	@echo "Installing compressor to /usr/local/bin..."
	@sudo cp bin/compressor /usr/local/bin/safetensors-compressor
	@echo "✓ Installed as 'safetensors-compressor'"

# Show help
help:
	@echo "SafeTensors Multi-Algorithm Compressor"
	@echo ""
	@echo "Build targets:"
	@echo "  make build       - Build the project"
	@echo "  make clean       - Remove all build artifacts"
	@echo "  make deps        - Install build dependencies (Ubuntu/Debian)"
	@echo "  make install     - Install binary system-wide"
	@echo ""
	@echo "Testing targets:"
	@echo "  make test        - Quick integrity test (ZSTD)"
	@echo "  make test-all    - Test all algorithms and modes"
	@echo ""
	@echo "Benchmarking targets:"
	@echo "  make benchmark   - Run comprehensive benchmark"
	@echo "  make compare     - Compare algorithms (balanced mode)"
	@echo "  make compare-fast   - Compare algorithms (fast mode)"
	@echo "  make compare-max    - Compare algorithms (maximum mode)"
	@echo ""
	@echo "Direct usage examples:"
	@echo "  # Compression"
	@echo "  ./bin/compressor compress model.safetensors out.stcmp zstd balanced"
	@echo "  ./bin/compressor compress model.safetensors out.stcmp lz4 fast"
	@echo "  ./bin/compressor compress model.safetensors out.stcmp lzma maximum"
	@echo ""
	@echo "  # Decompression (auto-detects algorithm)"
	@echo "  ./bin/compressor decompress out.stcmp model.safetensors"
	@echo ""
	@echo "  # Benchmarking"
	@echo "  ./bin/compressor benchmark model.safetensors"
	@echo "  ./bin/compressor compare model.safetensors balanced"
	@echo ""
	@echo "Algorithms: lz4, deflate, zstd, lzma"
	@echo "Modes: fast, balanced, maximum"
	@echo ""
# SafeTensors Multi-Algorithm Compressor with INT4/INT8 Quantization
# Supports: ZSTD, LZ4, DEFLATE, LZMA + Block-wise Quantization

.PHONY: all build clean clean-all test test-all benchmark compare help install deps \
        venv setup-python quantize-int4 quantize-int8 test-quantization \
        full-pipeline compare-all show-results

# Python virtual environment
VENV = venv
PYTHON = $(VENV)/bin/python
PIP = $(VENV)/bin/pip

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

# Clean build artifacts only
clean:
	@rm -rf build bin
	@echo "✓ Build artifacts cleaned!"

# Clean everything including output and quantized models
clean-all:
	@rm -rf build bin $(VENV)
	@rm -f output/*.json output/*.csv output/*.stcmp output/*.safetensors
	@rm -f test/model_int4.safetensors test/model_int8.safetensors
	@echo "✓ Full clean complete!"

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

# ============================================================================
# QUANTIZATION TARGETS
# ============================================================================

# Create Python virtual environment
venv:
	@echo "Creating Python virtual environment..."
	@python3 -m venv $(VENV)
	@echo "✓ Virtual environment created at ./$(VENV)"

# Setup Python environment with dependencies
setup-python: venv
	@echo "Installing Python dependencies..."
	@$(PIP) install --upgrade pip
	@$(PIP) install safetensors numpy torch --index-url https://download.pytorch.org/whl/cpu
	@echo "✓ Python dependencies installed!"

# Quantize model to INT4 (recommended - best compression)
quantize-int4: setup-python
	@echo "=== Running INT4 Block-wise Quantization ==="
	@$(PYTHON) scripts/quantize_blockwise.py test/model.safetensors test/model_int4.safetensors
	@echo ""
	@echo "✓ INT4 quantization complete!"
	@ls -lh test/model_int4.safetensors | awk '{print "  Size: " $$5}'
	@echo ""

# Quantize model to INT8 (better accuracy)
quantize-int8: setup-python
	@echo "=== Running INT8 Block-wise Quantization ==="
	@$(PYTHON) scripts/quantize_blockwise.py test/model.safetensors test/model_int8.safetensors --bits 8
	@echo ""
	@echo "✓ INT8 quantization complete!"
	@ls -lh test/model_int8.safetensors | awk '{print "  Size: " $$5}'
	@echo ""

# Test quantization with compression pipeline
test-quantization: build quantize-int4 quantize-int8
	@echo "=== Testing INT4 + ZSTD Pipeline ==="
	@./bin/compressor compress test/model_int4.safetensors output/model_int4.stcmp zstd maximum
	@echo ""
	@echo "=== Testing INT8 + ZSTD Pipeline ==="
	@./bin/compressor compress test/model_int8.safetensors output/model_int8.stcmp zstd maximum
	@echo ""
	@echo "✓ Quantization pipeline tests complete!"
	@echo ""

# Full pipeline: quantize + compress with all methods
full-pipeline: build quantize-int4 quantize-int8
	@echo "=== Running Full Compression Pipeline ==="
	@echo ""
	@echo "1. Lossless ZSTD-Maximum..."
	@./bin/compressor compress test/model.safetensors output/model_lossless.stcmp zstd maximum
	@echo ""
	@echo "2. INT8 + ZSTD-Maximum..."
	@./bin/compressor compress test/model_int8.safetensors output/model_int8.stcmp zstd maximum
	@echo ""
	@echo "3. INT4 + ZSTD-Maximum..."
	@./bin/compressor compress test/model_int4.safetensors output/model_int4.stcmp zstd maximum
	@echo ""
	@echo "✓ Full pipeline complete!"
	@echo ""
	@$(MAKE) show-results

# Compare all compression methods (lossless, INT8, INT4)
compare-all: full-pipeline
	@echo "=== COMPRESSION COMPARISON ==="
	@echo ""
	@echo "Method                Size      Ratio    Reduction"
	@echo "─────────────────────────────────────────────────────"
	@echo -n "Original (BFloat16)   "
	@ls -lh test/model.safetensors | awk '{printf "%-8s  1.00×    -\n", $$5}'
	@echo -n "Lossless (ZSTD-Max)   "
	@ls -lh output/model_lossless.stcmp | awk '{size=$$5; cmd="stat -c%s output/model_lossless.stcmp"; cmd | getline bytes; close(cmd); cmd2="stat -c%s test/model.safetensors"; cmd2 | getline orig; close(cmd2); ratio=orig/bytes; reduction=(1-bytes/orig)*100; printf "%-8s  %.2f×    %.1f%%\n", size, ratio, reduction}'
	@echo -n "INT8 + ZSTD-Max       "
	@ls -lh output/model_int8.stcmp | awk '{size=$$5; cmd="stat -c%s output/model_int8.stcmp"; cmd | getline bytes; close(cmd); cmd2="stat -c%s test/model.safetensors"; cmd2 | getline orig; close(cmd2); ratio=orig/bytes; reduction=(1-bytes/orig)*100; printf "%-8s  %.2f×    %.1f%%\n", size, ratio, reduction}'
	@echo -n "INT4 + ZSTD-Max       "
	@ls -lh output/model_int4.stcmp | awk '{size=$$5; cmd="stat -c%s output/model_int4.stcmp"; cmd | getline bytes; close(cmd); cmd2="stat -c%s test/model.safetensors"; cmd2 | getline orig; close(cmd2); ratio=orig/bytes; reduction=(1-bytes/orig)*100; printf "%-8s  %.2f×    %.1f%%\n", size, ratio, reduction}'
	@echo "─────────────────────────────────────────────────────"
	@echo ""

# Show current results
show-results:
	@echo "=== CURRENT RESULTS ==="
	@echo ""
	@if [ -f test/model.safetensors ]; then \
		echo "Original Model:"; \
		ls -lh test/model.safetensors | awk '{print "  " $$5 "  test/model.safetensors"}'; \
	fi
	@echo ""
	@if [ -f test/model_int4.safetensors ]; then \
		echo "Quantized Models:"; \
		ls -lh test/model_int4.safetensors test/model_int8.safetensors 2>/dev/null | awk '{print "  " $$5 "  " $$9}'; \
	fi
	@echo ""
	@if [ -f output/model_int4.stcmp ]; then \
		echo "Compressed Models:"; \
		ls -lh output/model_lossless.stcmp output/model_int8.stcmp output/model_int4.stcmp 2>/dev/null | awk '{print "  " $$5 "  " $$9}'; \
	fi
	@echo ""

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
	@echo "SafeTensors Multi-Algorithm Compressor with INT4/INT8 Quantization"
	@echo ""
	@echo "══════════════════════════════════════════════════════════════"
	@echo "BUILD & SETUP"
	@echo "══════════════════════════════════════════════════════════════"
	@echo "  make build           - Build C++ compressor"
	@echo "  make clean           - Remove build artifacts"
	@echo "  make clean-all       - Remove everything (build + venv + outputs)"
	@echo "  make deps            - Install C++ dependencies (Ubuntu/Debian)"
	@echo "  make setup-python    - Setup Python environment for quantization"
	@echo "  make install         - Install binary system-wide"
	@echo ""
	@echo "══════════════════════════════════════════════════════════════"
	@echo "LOSSLESS COMPRESSION (Perfect Quality)"
	@echo "══════════════════════════════════════════════════════════════"
	@echo "  make test            - Quick integrity test (ZSTD)"
	@echo "  make test-all        - Teapenas o endereço IP 192.1.1.100.st all algorithms and modes"
	@echo "  make benchmark       - Run comprehensive benchmark"
	@echo "  make compare         - Compare algorithms (balanced mode)"
	@echo "  make compare-fast    - Compare algorithms (fast mode)"
	@echo "  make compare-max     - Compare algorithms (maximum mode)"
	@echo ""
	@echo "══════════════════════════════════════════════════════════════"
	@echo "QUANTIZATION (Lossy Compression - Better Ratio)"
	@echo "══════════════════════════════════════════════════════════════"
	@echo "  make quantize-int4       - INT4 quantization (351 MB, ~98% quality)"
	@echo "  make quantize-int8       - INT8 quantization (530 MB, ~99% quality)"
	@echo "  make test-quantization   - Test quantization + compression"
	@echo "  make full-pipeline       - Run all methods (lossless, INT8, INT4)"
	@echo "  make compare-all         - Compare all compression methods"
	@echo "  make show-results        - Show current results"
	@echo ""
	@echo "══════════════════════════════════════════════════════════════"
	@echo "QUICK START EXAMPLES"
	@echo "══════════════════════════════════════════════════════════════"
	@echo ""
	@echo "Lossless compression:"
	@echo "  make build"
	@echo "  ./bin/compressor compress model.safetensors out.stcmp zstd maximum"
	@echo "  Result: 641 MB (1.47× compression, perfect quality)"
	@echo ""
	@echo "INT8 quantization + compression:"
	@echo "  make quantize-int8"
	@echo "  ./bin/compressor compress test/model_int8.safetensors out.stcmp zstd maximum"
	@echo "  Result: 530 MB (1.78× compression, ~99% quality)"
	@echo ""
	@echo "INT4 quantization + compression (RECOMMENDED):"
	@echo "  make quantize-int4"
	@echo "  ./bin/compressor compress test/model_int4.safetensors out.stcmp zstd maximum"
	@echo "  Result: 351 MB (2.69× compression, ~98% quality)"
	@echo ""
	@echo "Run everything and compare:"
	@echo "  make compare-all"
	@echo ""
	@echo "══════════════════════════════════════════════════════════════"
	@echo "COMPRESSION ALGORITHMS & MODES"
	@echo "══════════════════════════════════════════════════════════════"
	@echo "  Algorithms: lz4, deflate, zstd, lzma"
	@echo "  Modes: fast, balanced, maximum"
	@echo "  Quantization: INT4 (4-bit), INT8 (8-bit)"
	@echo ""
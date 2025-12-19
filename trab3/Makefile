# SafeTensors Compressor with INT4/INT8 Quantization
# Simple usage:
#   make compress FILE=model.safetensors QUANT=4 ALGO=zstd MODE=maximum
#   make decompress INPUT=output.stcmp OUTPUT=restored.safetensors

.PHONY: all build clean clean-all help deps setup-python compress decompress

# Python virtual environment
VENV = venv
PYTHON = $(VENV)/bin/python
PIP = $(VENV)/bin/pip

# User configuration - override these on command line
FILE ?= test/model.safetensors
OUTPUT ?= output/compressed.stcmp
INPUT ?= output/compressed.stcmp
RESTORED ?= output/restored.safetensors
QUANT ?= none
ALGO ?= zstd
MODE ?= maximum

# Default target
all: build

# ============================================================================
# CORE TARGETS - Simple Real-File Compression
# ============================================================================

# Compress a file with optional quantization
# Usage:
#   make compress FILE=mymodel.safetensors OUTPUT=mymodel.stcmp QUANT=4 ALGO=zstd MODE=maximum
#   make compress FILE=mymodel.safetensors QUANT=none ALGO=lz4 MODE=fast
compress: build
	@if [ ! -f "$(FILE)" ]; then \
		echo "Error: Input file '$(FILE)' not found!"; \
		exit 1; \
	fi
	@echo "=== Compressing $(FILE) ==="
	@echo "Algorithm: $(ALGO)"
	@echo "Mode: $(MODE)"
	@echo "Quantization: $(QUANT)"
	@echo ""
	@mkdir -p output /tmp/safetensors_tmp
	@# Check if quantization is requested
	@if [ "$(QUANT)" = "4" ] || [ "$(QUANT)" = "8" ]; then \
		echo "→ Step 1/2: Quantizing to INT$(QUANT)..."; \
		$(MAKE) setup-python > /dev/null 2>&1; \
		$(PYTHON) scripts/quantize_blockwise.py "$(FILE)" /tmp/safetensors_tmp/quantized_int$(QUANT).safetensors --bits $(QUANT); \
		echo ""; \
		echo "→ Step 2/2: Compressing with $(ALGO) ($(MODE))..."; \
		./bin/compressor compress /tmp/safetensors_tmp/quantized_int$(QUANT).safetensors "$(OUTPUT)" $(ALGO) $(MODE); \
	else \
		echo "→ Compressing directly (no quantization)..."; \
		./bin/compressor compress "$(FILE)" "$(OUTPUT)" $(ALGO) $(MODE); \
	fi
	@echo ""
	@echo "✓ Done! Output: $(OUTPUT)"
	@ls -lh "$(OUTPUT)" | awk '{print "  Size: " $$5}'
	@rm -rf /tmp/safetensors_tmp
	@echo ""

# Decompress a file
# Usage:
#   make decompress INPUT=compressed.stcmp OUTPUT=restored.safetensors
decompress: build
	@if [ ! -f "$(INPUT)" ]; then \
		echo "Error: Input file '$(INPUT)' not found!"; \
		exit 1; \
	fi
	@echo "=== Decompressing $(INPUT) ==="
	@mkdir -p output
	@./bin/compressor decompress "$(INPUT)" "$(RESTORED)"
	@echo ""
	@echo "✓ Done! Output: $(RESTORED)"
	@ls -lh "$(RESTORED)" | awk '{print "  Size: " $$5}'
	@echo ""

# ============================================================================
# BUILD & SETUP
# ============================================================================

# Build the C++ compressor
build:
	@echo "Building SafeTensors Compressor..."
	@mkdir -p build bin output
	@cd build && cmake .. && make -j$(shell nproc 2>/dev/null || echo 4)
	@echo ""
	@echo "✓ Build complete! Executable: ./bin/compressor"
	@echo ""

# Create Python virtual environment
venv:
	@echo "Creating Python virtual environment..."
	@python3 -m venv $(VENV)
	@echo "✓ Virtual environment created at ./$(VENV)"

# Setup Python environment with dependencies (for quantization)
setup-python: venv
	@if [ ! -f "$(VENV)/bin/safetensors" ] && [ ! -f "$(VENV)/lib/python"*"/site-packages/safetensors/__init__.py" ]; then \
		echo "Installing Python dependencies..."; \
		$(PIP) install --upgrade pip > /dev/null 2>&1; \
		$(PIP) install safetensors numpy torch --index-url https://download.pytorch.org/whl/cpu > /dev/null 2>&1; \
		echo "✓ Python dependencies installed!"; \
	fi

# Install C++ dependencies (Ubuntu/Debian)
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

# ============================================================================
# CLEANUP
# ============================================================================

# Clean build artifacts only
clean:
	@rm -rf build bin
	@echo "✓ Build artifacts cleaned!"

# Clean everything including outputs
clean-all:
	@rm -rf build bin $(VENV)
	@rm -f output/*.json output/*.csv output/*.stcmp output/*.safetensors
	@rm -rf /tmp/safetensors_tmp
	@echo "✓ Full clean complete!"

# ============================================================================
# TESTING & BENCHMARKING (Optional Advanced Targets)
# ============================================================================

# Quick integrity test
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

# Run comprehensive benchmark (built-in benchmarker)
benchmark: build
	@echo "=== Running Comprehensive Benchmark ==="
	@./bin/compressor benchmark test/model.safetensors
	@echo ""
	@echo "Results saved to:"
	@echo "  - output/benchmark_results.json"
	@echo "  - output/benchmark_results.csv"
	@echo ""

# Compare all compression methods (lossless, INT8, INT4)
compare-all: build setup-python
	@echo "=== Generating Comparison Report ==="
	@echo ""
	@# Quantize if needed
	@if [ ! -f test/model_int4.safetensors ]; then \
		echo "Quantizing to INT4..."; \
		$(PYTHON) scripts/quantize_blockwise.py test/model.safetensors test/model_int4.safetensors --bits 4; \
	fi
	@if [ ! -f test/model_int8.safetensors ]; then \
		echo "Quantizing to INT8..."; \
		$(PYTHON) scripts/quantize_blockwise.py test/model.safetensors test/model_int8.safetensors --bits 8; \
	fi
	@echo ""
	@# Compress all versions
	@echo "Compressing lossless (ZSTD-Maximum)..."
	@./bin/compressor compress test/model.safetensors output/model_lossless.stcmp zstd maximum > /dev/null
	@echo "Compressing INT8 (ZSTD-Maximum)..."
	@./bin/compressor compress test/model_int8.safetensors output/model_int8.stcmp zstd maximum > /dev/null
	@echo "Compressing INT4 (ZSTD-Maximum)..."
	@./bin/compressor compress test/model_int4.safetensors output/model_int4.stcmp zstd maximum > /dev/null
	@echo ""
	@# Show results
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

# ============================================================================
# HELP
# ============================================================================

help:
	@echo "═══════════════════════════════════════════════════════════════════════"
	@echo "SafeTensors Compressor - Simple Real-File Compression Tool"
	@echo "═══════════════════════════════════════════════════════════════════════"
	@echo ""
	@echo "BASIC USAGE"
	@echo "───────────────────────────────────────────────────────────────────────"
	@echo ""
	@echo "1. Lossless compression (no quantization):"
	@echo "   make compress FILE=mymodel.safetensors OUTPUT=mymodel.stcmp"
	@echo ""
	@echo "2. Lossy compression with INT4 quantization (best compression):"
	@echo "   make compress FILE=mymodel.safetensors OUTPUT=mymodel.stcmp QUANT=4"
	@echo ""
	@echo "3. Lossy compression with INT8 quantization (better quality):"
	@echo "   make compress FILE=mymodel.safetensors OUTPUT=mymodel.stcmp QUANT=8"
	@echo ""
	@echo "4. Decompress any compressed file:"
	@echo "   make decompress INPUT=mymodel.stcmp OUTPUT=restored.safetensors"
	@echo ""
	@echo "═══════════════════════════════════════════════════════════════════════"
	@echo "PARAMETERS"
	@echo "───────────────────────────────────────────────────────────────────────"
	@echo ""
	@echo "FILE     - Input SafeTensors file to compress"
	@echo "OUTPUT   - Output compressed file path (default: output/compressed.stcmp)"
	@echo "INPUT    - Input compressed file for decompression"
	@echo "RESTORED - Output restored file path (default: output/restored.safetensors)"
	@echo ""
	@echo "QUANT    - Quantization mode:"
	@echo "           none - No quantization, lossless (default)"
	@echo "           4    - INT4 quantization (351 MB, ~98%% quality)"
	@echo "           8    - INT8 quantization (530 MB, ~99%% quality)"
	@echo ""
	@echo "ALGO     - Compression algorithm (default: zstd)"
	@echo "           lz4     - Ultra-fast, lower ratio"
	@echo "           deflate - Standard gzip, widely compatible"
	@echo "           zstd    - Best overall balance (RECOMMENDED)"
	@echo "           lzma    - Maximum ratio, very slow"
	@echo ""
	@echo "MODE     - Compression mode (default: maximum)"
	@echo "           fast    - Fastest compression"
	@echo "           balanced- Good speed/ratio trade-off"
	@echo "           maximum - Best compression ratio"
	@echo ""
	@echo "═══════════════════════════════════════════════════════════════════════"
	@echo "EXAMPLES"
	@echo "───────────────────────────────────────────────────────────────────────"
	@echo ""
	@echo "# Lossless fast compression"
	@echo "make compress FILE=model.safetensors ALGO=zstd MODE=fast"
	@echo ""
	@echo "# INT4 maximum compression (best ratio)"
	@echo "make compress FILE=model.safetensors QUANT=4 ALGO=zstd MODE=maximum"
	@echo ""
	@echo "# INT8 with LZ4 (fast, good quality)"
	@echo "make compress FILE=model.safetensors QUANT=8 ALGO=lz4 MODE=fast"
	@echo ""
	@echo "# LZMA for archival (very slow, max compression)"
	@echo "make compress FILE=model.safetensors ALGO=lzma MODE=maximum"
	@echo ""
	@echo "# Decompress"
	@echo "make decompress INPUT=output/compressed.stcmp OUTPUT=restored.safetensors"
	@echo ""
	@echo "═══════════════════════════════════════════════════════════════════════"
	@echo "BUILD & SETUP"
	@echo "───────────────────────────────────────────────────────────────────────"
	@echo "  make build        - Build C++ compressor"
	@echo "  make deps         - Install C++ dependencies (Ubuntu/Debian)"
	@echo "  make setup-python - Setup Python environment (needed for quantization)"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make clean-all    - Remove everything (build + outputs)"
	@echo ""
	@echo "═══════════════════════════════════════════════════════════════════════"
	@echo "TESTING & BENCHMARKING (Optional)"
	@echo "───────────────────────────────────────────────────────────────────────"
	@echo "  make test         - Quick integrity test"
	@echo "  make test-all     - Test all algorithms and modes"
	@echo "  make benchmark    - Run comprehensive benchmark"
	@echo "  make compare-all  - Compare lossless, INT8, INT4 compression"
	@echo ""
	@echo "═══════════════════════════════════════════════════════════════════════"
	@echo "EXPECTED RESULTS (Qwen2-0.5B, 943 MB model)"
	@echo "───────────────────────────────────────────────────────────────────────"
	@echo "  Lossless ZSTD:    641 MB (1.47×) - Perfect quality"
	@echo "  INT8 + ZSTD:      530 MB (1.78×) - ~99%% quality"
	@echo "  INT4 + ZSTD:      351 MB (2.69×) - ~98%% quality"
	@echo ""
	@echo "═══════════════════════════════════════════════════════════════════════"
	@echo ""

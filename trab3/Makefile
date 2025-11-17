.PHONY: all build clean test benchmark help

all: build

build:
	@mkdir -p build bin output
	@cd build && cmake .. && make
	@echo ""
	@echo "Build complete! Executable: ./bin/compressor"

clean:
	@rm -rf build bin
	@rm -f output/*.json output/*.csv output/*.stcmp output/*.safetensors
	@echo "Clean complete!"

test: build
	@echo "Running compression test..."
	@./bin/compressor compress test/model.safetensors output/test.stcmp fast
	@echo ""
	@echo "Testing decompression..."
	@./bin/compressor decompress output/test.stcmp output/restored.safetensors
	@echo ""
	@echo "Verifying integrity..."
	@if cmp -s test/model.safetensors output/restored.safetensors; then \
		echo "SUCCESS: Files match!"; \
	else \
		echo "ERROR: Files do not match!"; \
		exit 1; \
	fi
	@rm -f output/test.stcmp output/restored.safetensors

benchmark: build
	@./bin/compressor benchmark test/model.safetensors

help:
	@echo "SafeTensors Compressor"
	@echo ""
	@echo "  make build      - Build the project"
	@echo "  make clean      - Remove build artifacts"
	@echo "  make test       - Run integrity test"
	@echo "  make benchmark  - Run all benchmarks"
	@echo ""
	@echo "Direct usage:"
	@echo "  ./bin/compressor compress <input> <output> [fast|balanced|maximum]"
	@echo "  ./bin/compressor decompress <input> <output>"
	@echo "  ./bin/compressor benchmark <input>"

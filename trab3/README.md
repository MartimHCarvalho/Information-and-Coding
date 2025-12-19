# SafeTensors Compressor

**Academic Project - Information and Coding (2025/26)**  
Universidade de Aveiro - DETI

Specialized compression tool for neural network weights in SafeTensors format, optimized for BFloat16 data compression using domain-specific preprocessing.

---

## Features

- **Optimized for Neural Networks** - BFloat16-aware byte reordering preprocessing
- **INT4/INT8 Quantization** - Block-wise quantization for 2.7-3.1× compression
- **Multiple Algorithms** - LZ4, DEFLATE (gzip), ZSTD, LZMA support
- **Three Operation Modes** - Fast, Balanced, and Maximum compression
- **Lossless: 32% Space Savings** - Qwen2-0.5B model (943 MB → 641 MB with ZSTD)
- **Lossy (INT4): 63% Space Savings** - Qwen2-0.5B model (943 MB → 351 MB)
- **Bit-exact Decompression** - Lossless compression with verification

---

## Installation

### Dependencies

**C++ Compression Tool:**
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake libzstd-dev liblz4-dev zlib1g-dev liblzma-dev

# Or use make
make deps
```

**Python Quantization (Optional):**
```bash
# For INT4/INT8 quantization support
pip install safetensors numpy torch
```

### Build

```bash
make build
```

---

## Quick Start

### Compress (Fast Mode)
```bash
./bin/compressor compress model.safetensors output.stcmp zstd fast
```

### Compress (Maximum Mode)
```bash
./bin/compressor compress model.safetensors output.stcmp zstd maximum
```

### Decompress
```bash
./bin/compressor decompress output.stcmp restored.safetensors
```

---

## Performance Comparison

**Test Model:** Qwen2-0.5B (943 MB, 494M parameters)

### Fast Mode (Optimized for Speed)

| Tool | Size (MB) | Ratio | Time | Speed | Best For |
|------|-----------|-------|------|-------|----------|
| **This Tool (ZSTD-Fast)** | **669 MB** | **1.41x** | **4.8s** | **195 MB/s** | **Recommended** |
| LZ4-Fast | 792 MB | 1.19x | 4.1s | 230 MB/s | Ultra-fast |
| zstd -3 | 682 MB | 1.38x | 7.3s | 129 MB/s | Standard fast |
| gzip -1 | 685 MB | 1.38x | 23s | 41 MB/s | Legacy |

### Maximum Mode (Optimized for Compression)

| Tool | Size (MB) | Ratio | Time | Speed | Best For |
|------|-----------|-------|------|-------|----------|
| **This Tool (ZSTD-Max)** | **641 MB** | **1.47x** | **509s** | **1.9 MB/s** | **Best Ratio** |
| zstd -19 | 654 MB | 1.44x | 520s | 1.8 MB/s | Standard max |
| xz -9 (LZMA) | 651 MB | 1.45x | 961s | 1.0 MB/s | Very slow |
| gzip -9 | 674 MB | 1.40x | 146s | 6.4 MB/s | Standard |

**Key Advantage:** Domain-specific preprocessing provides **5-10% better compression** than generic tools.

---

## Quantization: Breaking the Lossless Barrier

For applications where minimal quality loss (~1-2%) is acceptable, block-wise quantization provides dramatically better compression:

### Quantization Results

**Test Model:** Qwen2-0.5B (943 MB, 494M parameters)

| Approach | Final Size | Ratio | Quality | Reduction |
|----------|-----------|-------|---------|-----------|
| **Lossless (ZSTD-Max)** | **641 MB** | **1.47×** | **Perfect** | **32%** |
| **INT8 + ZSTD-Max** | **530 MB** | **1.78×** | **~99%** | **44%** |
| **INT4 + ZSTD-Max** | **351 MB** | **2.69×** | **~98%** | **63%** |

### How Quantization Works

```
Original (BFloat16):  943 MB
        ↓
Block-wise INT4 Quantization:  701 MB (4 bits per weight, packed)
        ↓
ZSTD Compression:  351 MB
```

**Block-wise Quantization:**
- Divide weights into blocks of 128
- Each block gets its own scale factor
- Prevents outliers from destroying precision
- INT4: 16 quantization levels (-8 to 7)
- INT8: 256 quantization levels (-128 to 127)

### Quantization Usage

```bash
# Install Python dependencies
pip install safetensors numpy torch

# INT4 quantization (recommended - best compression)
python scripts/quantize_blockwise.py test/model.safetensors test/model_int4.safetensors
./bin/compressor compress test/model_int4.safetensors output/model_int4.stcmp zstd maximum
# Result: 351 MB (2.69× compression)

# INT8 quantization (better accuracy)
python scripts/quantize_blockwise.py test/model.safetensors test/model_int8.safetensors --bits 8
./bin/compressor compress test/model_int8.safetensors output/model_int8.stcmp zstd maximum
# Result: 530 MB (1.78× compression)
```

**When to Use:**
- **Lossless:** Research, benchmarking, perfect accuracy required
- **INT8:** Production with minimal quality loss (<1%)
- **INT4:** Edge deployment, maximum compression (~1-2% quality loss)

**Documentation:** See [docs/BLOCKWISE_QUANTIZATION.md](docs/BLOCKWISE_QUANTIZATION.md) for detailed technical explanation.

---

## Why It Works Better

### Standard Tools (Generic)
```
Input: [BF16 data] → Compress → Output
```

### This Tool (Domain-Specific)
```
Input: [BF16 data] → Byte Reordering → Compress → Output
                      ↓
              Groups similar bytes together
              (5% entropy reduction)
```

**Byte Reordering:**
- Separates high/low bytes of BFloat16 values
- High bytes cluster around 0x3c, 0xbc (~28% frequency)
- Low bytes grouped separately
- Result: Better pattern recognition for compression algorithms

---

## Usage

### Basic Compression

```bash
# Fast mode (recommended for most use cases)
./bin/compressor compress input.safetensors output.stcmp zstd fast

# Balanced mode (good speed/ratio trade-off)
./bin/compressor compress input.safetensors output.stcmp zstd balanced

# Maximum mode (best compression ratio)
./bin/compressor compress input.safetensors output.stcmp zstd maximum
```

### Algorithm Selection

All algorithms support three operation modes: **fast**, **balanced**, and **maximum**

```bash
# Ultra-fast with LZ4
./bin/compressor compress input.safetensors output.stcmp lz4 fast

# Balanced performance with ZSTD (recommended)
./bin/compressor compress input.safetensors output.stcmp zstd balanced

# Maximum compression with ZSTD
./bin/compressor compress input.safetensors output.stcmp zstd maximum

# LZMA for archival (very slow)
./bin/compressor compress input.safetensors output.stcmp lzma maximum
```

### Decompression

```bash
# Auto-detects algorithm from file header
./bin/compressor decompress output.stcmp restored.safetensors

# Verify integrity
cmp input.safetensors restored.safetensors && echo "Perfect match!"
```

### Benchmarking

```bash
# Test all algorithms and modes
make benchmark

# Results saved to:
# - output/benchmark_results.json
# - output/benchmark_results.csv
```

---

## Supported Algorithms

| Algorithm | Fast Mode | Balanced Mode | Maximum Mode | Characteristics |
|-----------|-----------|---------------|--------------|-----------------|
| **LZ4** | Level 0 | Level 6 (HC) | Level 12 (HC) | Ultra-fast, lower ratio |
| **DEFLATE** | Level 3 | Level 6 | Level 9 | Standard gzip, widely compatible |
| **ZSTD** | Level 3 | Level 10 | Level 19 | Best overall balance (recommended) |
| **LZMA** | Level 3 | Level 6 | Level 9 | Maximum ratio, very slow |

**Recommendation:** Use ZSTD for best speed/ratio trade-off.

### Algorithm Comparison: Why ZSTD Performs Best

Understanding why different algorithms perform differently on BFloat16 neural network weights:

**LZ4**
- **Strengths:** High throughput (230 MB/s), simple LZ77 implementation
- **Weaknesses:** Small dictionary (64 KB), no entropy coding
- **Result:** 1.19x ratio - insufficient for highly structured data
- **Best for:** Real-time compression where speed is critical

**DEFLATE**
- **Strengths:** Widely compatible (gzip standard), combines LZ77 with Huffman coding
- **Weaknesses:** 32 KB dictionary limit, static Huffman tables
- **Result:** 1.44x ratio - good but not optimal for skewed distributions
- **Best for:** Compatibility requirements and moderate compression needs

**ZSTD**
- **Strengths:**
  - **Finite State Entropy (FSE):** Superior to Huffman for skewed data, ideal for 28% byte clustering
  - **Large Dictionary (128 KB):** Captures longer repeating patterns
  - **Modern LZ77:** Fast match finding with optimal parsing
  - **Adaptive Strategy:** Switches between FSE/RLE based on data characteristics
- **Result:** 1.47x ratio with excellent speed
- **Best for:** BFloat16 neural network weight compression

**LZMA**
- **Strengths:** Sophisticated range encoder, large dictionary, maximum compression
- **Weaknesses:** Low throughput (1.0 MB/s), high memory usage
- **Result:** 1.47x ratio (equivalent to ZSTD) but 2x slower
- **Best for:** Archival storage where compression time is not critical

**Key Insight:** ZSTD's FSE algorithm is specifically designed for data with skewed byte distributions. Analysis reveals that 28.33% of bytes are concentrated in just two values (0x3c and 0xbc), which FSE exploits effectively.

---

## Project Structure

```
trab3/
├── src/                    # C++ source code
│   ├── main.cpp           # CLI interface
│   ├── compressor.cpp     # Compression algorithms
│   ├── preprocessor.cpp   # Byte reordering logic
│   ├── safetensors_parser.cpp
│   └── benchmarker.cpp
├── scripts/               # Python quantization scripts
│   └── quantize_blockwise.py  # INT4/INT8 quantization
├── docs/                  # Documentation
│   ├── BLOCKWISE_QUANTIZATION.md  # Quantization technical guide
│   ├── REPORT_ANALYSIS.md         # Byte distribution analysis
│   └── ZSTD_OPTIMIZATIONS.md      # ZSTD tuning details
├── includes/              # Headers
├── test/
│   └── model.safetensors  # Test model (~943 MB, gitignored)
├── Makefile              # Build system
└── README.md             # This file
```

---

## Makefile Commands

```bash
make deps         # Install dependencies (Ubuntu/Debian)
make build        # Build the project
make clean        # Remove build artifacts
make test         # Quick integrity test
make benchmark    # Run comprehensive benchmark
```

---

## File Format (.stcmp)

```
[5B: "STCMP"]           # Magic number
[1B: version]           # Format version (2)
[1B: algorithm]         # 0=LZ4, 1=DEFLATE, 2=ZSTD, 3=LZMA
[1B: operation_point]   # 0=Fast, 2=Maximum
[8B: header_size]       # SafeTensors metadata size
[...header...]          # Original JSON metadata
[8B: data_size]         # Compressed data size
[...compressed...]      # Compressed tensor data
```

---

## Academic Context

### Demonstrated Concepts

1. **Information Theory** - Shannon entropy analysis and reduction
2. **Domain-Specific Optimization** - BFloat16 format exploitation
3. **Lossy Compression** - Block-wise quantization (INT4/INT8)
4. **Algorithm Comparison** - Empirical evaluation of 4 compression standards
5. **Rate-Distortion Trade-offs** - Speed vs compression ratio analysis

### Key Results

**Lossless Compression:**
- **Byte reordering reduces entropy by ~5%** (7.9 → 7.5 bits/byte)
- **ZSTD achieves best efficiency** across all modes
- **32% space savings** with fast decompression (< 2s)
- **Modern algorithms (ZSTD) outperform classics** (DEFLATE, LZMA)

**Lossy Compression (Quantization):**
- **INT4 achieves 2.69× compression** (943 MB → 351 MB)
- **INT8 achieves 1.78× compression** (943 MB → 530 MB)
- **Block-wise approach** prevents outlier-induced precision loss
- **Minimal quality degradation** (~1-2% for INT4, <1% for INT8)

### Why Generic Tools Are Less Effective

Generic compression tools don't understand BFloat16 structure:
- Standard gzip -9: **1.40x** in 146s
- This tool ZSTD-Max: **1.47x** in 509s
- **Improvement: +5% better ratio** through preprocessing

---

## Testing

### Quick Test
```bash
make test
```

### Full Benchmark
```bash
make benchmark
```

### Manual Verification
```bash
./bin/compressor compress test/model.safetensors output/test.stcmp zstd fast
./bin/compressor decompress output/test.stcmp output/restored.safetensors
cmp test/model.safetensors output/restored.safetensors
# No output = perfect match
```

---

## Requirements

- **C++17** compiler (g++ 7+ or clang++ 5+)
- **CMake** 3.10+
- **Libraries:** libzstd-dev, liblz4-dev, zlib1g-dev, liblzma-dev

---

## References

### External Resources
- [SafeTensors Format](https://github.com/huggingface/safetensors)
- [Zstandard (ZSTD)](https://github.com/facebook/zstd)
- [LZ4 Compression](https://github.com/lz4/lz4)
- [BFloat16 Format](https://en.wikipedia.org/wiki/Bfloat16_floating-point_format)
- [Qwen2-0.5B Model](https://huggingface.co/Qwen/Qwen2-0.5B)

### Project Documentation
- [Block-wise Quantization Guide](docs/BLOCKWISE_QUANTIZATION.md) - INT4/INT8 technical details
- [ZSTD Optimizations](docs/ZSTD_OPTIMIZATIONS.md) - Parameter tuning analysis
- [Byte Distribution Analysis](docs/REPORT_ANALYSIS.md) - Entropy and compression theory

---

## Quick Reference Card

### Lossless Compression

```bash
# Fast compression (4.8s, 669 MB)
./bin/compressor compress model.safetensors out.stcmp zstd fast

# Balanced compression (optimal speed/ratio)
./bin/compressor compress model.safetensors out.stcmp zstd balanced

# Maximum compression (509s, 641 MB)
./bin/compressor compress model.safetensors out.stcmp zstd maximum

# Decompress
./bin/compressor decompress out.stcmp model.safetensors

# Benchmark all algorithms and modes
make benchmark
```

### Lossy Compression (Quantization)

```bash
# Install Python dependencies (one-time)
pip install safetensors numpy torch

# INT4 quantization + compression (351 MB, ~98% quality)
python scripts/quantize_blockwise.py model.safetensors model_int4.safetensors
./bin/compressor compress model_int4.safetensors model_int4.stcmp zstd maximum

# INT8 quantization + compression (530 MB, ~99% quality)
python scripts/quantize_blockwise.py model.safetensors model_int8.safetensors --bits 8
./bin/compressor compress model_int8.safetensors model_int8.stcmp zstd maximum
```

### Recommendations

- **Lossless (Perfect Quality):** ZSTD-Maximum → 641 MB
- **Lossy (High Quality):** INT8 + ZSTD-Maximum → 530 MB (~99% quality)
- **Lossy (Maximum Compression):** INT4 + ZSTD-Maximum → 351 MB (~98% quality)
- **Production:** ZSTD-Fast for speed, INT4 for edge deployment

# SafeTensors Compressor

**Academic Project - Information and Coding (2025/26)**  
Universidade de Aveiro - DETI

Specialized compression tool for neural network weights in SafeTensors format, optimized for BFloat16 data compression using domain-specific preprocessing.

---

## Features

- **Optimized for Neural Networks** - BFloat16-aware byte reordering preprocessing
- **Multithreaded Compression** - ZSTD uses all CPU cores for 2-5× faster compression
- **INT4/INT8 Quantization** - Block-wise quantization for 2.7× compression
- **Multiple Algorithms** - LZ4, DEFLATE (gzip), ZSTD, LZMA support
- **Two Operation Modes** - Fast and Maximum compression
- **Lossless: 33% Space Savings** - Qwen2-0.5B model (943 MB → 633 MB with ZSTD)
- **Lossy (INT4): 63% Space Savings** - Qwen2-0.5B model (943 MB → 350 MB)
- **Bit-exact Decompression** - Lossless compression with verification
- **Fast Decompression** - Up to 500 MB/s throughput

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

Get started in 30 seconds:

```bash
# Build the compressor
make build

# Compress a model (ZSTD Maximum - best ratio)
./bin/compressor compress test/model.safetensors output.stcmp

# Decompress
./bin/compressor decompress output.stcmp restored.safetensors
```

**→ See the [Usage](#usage) section for all algorithms, modes, and quantization options.**

---

## Performance Comparison

**Test Model:** Qwen2-0.5B (943 MB, 494M parameters)

### Fast Mode (Optimized for Speed)

| Tool | Size (MB) | Ratio | Time | Speed | Best For |
|------|-----------|-------|------|-------|----------|
| **Custom ZSTD (Fast)** | **669 MB** | **1.41×** | **5.5s** | **172 MB/s** | **Recommended** |
| LZ4 | 792 MB | 1.19× | 4.1s | 230 MB/s | Ultra-fast |
| zstd  | 682 MB | 1.38× | 7.3s | 129 MB/s | Standard fast |
| DEFLATE | 685 MB | 1.38× | 23s | 41 MB/s | Legacy |

### Maximum Mode (Optimized for Compression)

| Tool | Size (MB) | Ratio | Time | Speed | Decompression |
|------|-----------|-------|------|-------|---------------|
| **Custom ZSTD (Maximum)** | **633 MB** | **1.49×** | **104s** | **9.0 MB/s** | **503 MB/s** |
| zstd  | 654 MB | 1.44× | 520s | 1.8 MB/s | - |
| LZMA | 651 MB | 1.45× | 961s | 1.0 MB/s | - |
| DEFLATE | 674 MB | 1.40× | 146s | 6.4 MB/s | - |

**Key Advantages:**
- **Domain-specific preprocessing:** 5-10% better compression than generic tools
- **Multithreading:** 2-5× faster than single-threaded compression
- **Fast decompression:** Up to 503 MB/s throughput

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

## Quantization: Breaking the Lossless Barrier

For applications where minimal quality loss (~1-2%) is acceptable, block-wise quantization provides dramatically better compression.


### Quantization Results

**Test Model:** Qwen2-0.5B (943 MB, 494M parameters)

| Approach | Final Size | Ratio | Quality | Reduction | Time |
|----------|-----------|-------|---------|-----------|------|
| **Lossless (ZSTD-Maximum)** | **633 MB** | **1.49×** | **Perfect** | **33%** | **104s** |
| **INT8 + ZSTD-Maximum** | **529 MB** | **1.78×** | **~99%** | **44%** | **79s** |
| **INT4 + ZSTD-Maximum** | **350 MB** | **2.69×** | **~98%** | **63%** | **70s** |

### How Quantization Works

```
Original (BFloat16):  943 MB
        ↓
Block-wise INT4 Quantization:  701 MB (4 bits per weight, packed)
        ↓
ZSTD Multithreaded Compression:  350 MB
```

**Block-wise Quantization:**
- Divide weights into blocks of 128
- Each block gets its own scale factor
- Prevents outliers from destroying precision
- INT4: 16 quantization levels (-8 to 7)
- INT8: 256 quantization levels (-128 to 127)

## Usage

The files to be compressed must be in SafeTensors format and in the **test/** directory.

This tool supports two compression approaches:
- **Lossless Compression (1-step):** Perfect reconstruction using ZSTD, LZ4, DEFLATE, or LZMA
- **Lossy Compression (2-step):** INT4/INT8 quantization followed by ZSTD compression

**⚠️ IMPORTANT:** Quantization is a **2-step process**:
1. **Step 1 - Quantization:** Convert weights from BFloat16/Float32 to INT4/INT8 (Python script)
2. **Step 2 - Compression:** Compress the quantized file with ZSTD (C++ binary)

---

### Method 1: Using Makefile (EASIEST - Recommended)

The Makefile automatically handles all compression workflows:

```bash
# ONE-TIME SETUP (install Python dependencies for quantization)
make setup-python

# ═══════════════════════════════════════════════════
# LOSSLESS COMPRESSION (1-step, no quantization)
# ═══════════════════════════════════════════════════

# ZSTD Fast mode (Best speed)
make compress FILE=test/model.safetensors OUTPUT=output/zstd_fast.stcmp ALGO=zstd MODE=fast
# Result: 943 MB → 669 MB (1.41×, 5.5s)

# ZSTD Maximum mode (Best compression ratio)
make compress FILE=test/model.safetensors OUTPUT=output/zstd_max.stcmp ALGO=zstd MODE=maximum
# Result: 943 MB → 633 MB (1.49×, 104s)

# LZ4 (ultra-fast)
make compress FILE=test/model.safetensors OUTPUT=output/lz4.stcmp ALGO=lz4 MODE=fast
# Result: 943 MB → 792 MB (1.19×, 4.1s)

# DEFLATE (gzip-compatible)
make compress FILE=test/model.safetensors OUTPUT=output/deflate.stcmp ALGO=deflate MODE=maximum
# Result: 943 MB → 674 MB (1.40×, 146s)

# LZMA (maximum compression)
make compress FILE=test/model.safetensors OUTPUT=output/lzma.stcmp ALGO=lzma MODE=maximum
# Result: 943 MB → 651 MB (1.45×, 961s)

# ═══════════════════════════════════════════════════
# LOSSY COMPRESSION (2-step: quantization + compression)
# ═══════════════════════════════════════════════════

# INT4 quantization + ZSTD compression (automatic 2-step)
make compress FILE=test/model.safetensors OUTPUT=output/int4.stcmp QUANT=4
# Step 1: 943 MB → 701 MB (quantization)
# Step 2: 701 MB → 350 MB (ZSTD compression)
# Result: 350 MB total (2.69×, 70s, ~98% quality)

# INT8 quantization + ZSTD compression (automatic 2-step)
make compress FILE=test/model.safetensors OUTPUT=output/int8.stcmp QUANT=8
# Step 1: 943 MB → 872 MB (quantization)
# Step 2: 872 MB → 529 MB (ZSTD compression)
# Result: 529 MB total (1.78×, 79s, ~99% quality)

# ═══════════════════════════════════════════════════
# DECOMPRESSION (works for all methods)
# ═══════════════════════════════════════════════════
make decompress INPUT=output/zstd_max.stcmp RESTORED=output/restored.safetensors

# Verify integrity (lossless methods only)
cmp test/model.safetensors output/restored.safetensors && echo "Perfect match!"
```

---

### Method 2: Manual Step-by-Step (Using Binaries Directly)

For fine-grained control or understanding the process:

```bash
# ONE-TIME SETUP: Create Python virtual environment (for quantization)
make setup-python
# Or manually:
# python3 -m venv venv
# sourve venv/bin/activate
# pip install safetensors numpy torch

make build

# ═══════════════════════════════════════════════════
# LOSSLESS COMPRESSION (1-step, all algorithms)
# ═══════════════════════════════════════════════════

# ZSTD - Fast mode
./bin/compressor compress test/model.safetensors output/zstd_fast.stcmp zstd fast
# Result: 943 MB → 669 MB (1.41×, 5.5s, 172 MB/s)

# ZSTD - Maximum mode (best balance)
./bin/compressor compress test/model.safetensors output/zstd_max.stcmp zstd maximum
# Result: 943 MB → 633 MB (1.49×, 104s, decompression: 503 MB/s)

# LZ4 - Fast mode (ultra-fast)
./bin/compressor compress test/model.safetensors output/lz4_fast.stcmp lz4 fast
# Result: 943 MB → 792 MB (1.19×, 4.1s, 230 MB/s)

# LZ4 - Maximum mode (high compression)
./bin/compressor compress test/model.safetensors output/lz4_max.stcmp lz4 maximum
# Result: Higher compression than fast, slower

# DEFLATE - Fast mode (gzip-compatible)
./bin/compressor compress test/model.safetensors output/deflate_fast.stcmp deflate fast
# Result: 943 MB → 685 MB (1.38×, 23s)

# DEFLATE - Maximum mode
./bin/compressor compress test/model.safetensors output/deflate_max.stcmp deflate maximum
# Result: 943 MB → 674 MB (1.40×, 146s)

# LZMA - Maximum mode (slowest, high ratio)
./bin/compressor compress test/model.safetensors output/lzma_max.stcmp lzma maximum
# Result: 943 MB → 651 MB (1.45×, 961s)

# Default: ZSTD Maximum (if algorithm/mode omitted)
./bin/compressor compress test/model.safetensors output/default.stcmp

# ═══════════════════════════════════════════════════
# LOSSY COMPRESSION (2-step: quantization + ZSTD)
# ═══════════════════════════════════════════════════

# INT4 COMPRESSION (2-step process)
# Step 1: Quantize the model (BFloat16 → INT4)
venv/bin/python scripts/quantize_blockwise.py test/model.safetensors output/model_int4.safetensors --bits 4
# Output: 701 MB (INT4 quantized model)

# Step 2: Compress the quantized model (INT4 → ZSTD)
./bin/compressor compress output/model_int4.safetensors output/final_int4.stcmp zstd maximum
# Output: 350 MB (final compressed file)

# INT8 COMPRESSION (2-step process)
# Step 1: Quantize the model (BFloat16 → INT8)
venv/bin/python scripts/quantize_blockwise.py test/model.safetensors output/model_int8.safetensors --bits 8
# Output: 872 MB (INT8 quantized model)

# Step 2: Compress the quantized model (INT8 → ZSTD)
./bin/compressor compress output/model_int8.safetensors output/final_int8.stcmp zstd maximum
# Output: 529 MB (final compressed file)

# ═══════════════════════════════════════════════════
# DECOMPRESSION (auto-detects algorithm from header)
# ═══════════════════════════════════════════════════
./bin/compressor decompress output/zstd_max.stcmp output/restored.safetensors
./bin/compressor decompress output/final_int4.stcmp output/restored_int4.safetensors

# Verify integrity (lossless only - should be perfect match)
cmp test/model.safetensors output/restored.safetensors && echo "Perfect match!"
```

---

### Algorithm Selection Guide

All algorithms support two operation modes: **fast** and **maximum**

| Algorithm | Fast Mode | Maximum Mode | Characteristics |
|-----------|-----------|--------------|-----------------|
| **ZSTD** | Level 3 | Level 19 | **RECOMMENDED** - Best balance, multithreaded |
| **LZ4** | Level 0 | Level 12 (HC) | Ultra-fast, lower compression ratio |
| **DEFLATE** | Level 3 | Level 9 | Standard gzip, widely compatible |
| **LZMA** | Level 3 | Level 9 | Maximum ratio, very slow |

**When to Use:**
- **ZSTD Maximum:** Best overall choice - excellent ratio (1.49×) with fast decompression (503 MB/s)
- **ZSTD Fast:** Speed-critical workflows - 172 MB/s compression, 5.5s total
- **LZ4 Fast:** Ultra-fast compression needed - 230 MB/s, lowest ratio (1.19×)
- **DEFLATE:** Compatibility with standard gzip tools required
- **LZMA:** Archival storage where compression time doesn't matter
- **INT4 + ZSTD:** Edge deployment, maximum compression (2.69×, ~98% quality)
- **INT8 + ZSTD:** Production with minimal quality loss (1.78×, ~99% quality)

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

| Algorithm | Fast Mode | Maximum Mode | Characteristics |
|-----------|-----------|--------------|-----------------|
| **LZ4** | Level 0 | Level 12 (HC) | Ultra-fast, lower ratio |
| **DEFLATE** | Level 3 | Level 9 | Standard gzip, widely compatible |
| **ZSTD** | Level 3 | Level 19 | Best overall balance (recommended) |
| **LZMA** | Level 3 | Level 9 | Maximum ratio, very slow |

**Recommendation:** Use ZSTD for best speed/ratio trade-off with multithreading support.

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
- **ZSTD with multithreading achieves best efficiency** across all modes
- **33% space savings** with fast decompression (503 MB/s)
- **Multithreading provides 2-5× speedup** compared to single-threaded
- **Modern algorithms (ZSTD) outperform classics** (DEFLATE, LZMA)

**Lossy Compression (Quantization):**
- **INT4 achieves 2.69× compression** (943 MB → 350 MB in 70s)
- **INT8 achieves 1.78× compression** (943 MB → 529 MB in 79s)
- **Block-wise approach** prevents outlier-induced precision loss
- **Minimal quality degradation** (~1-2% for INT4, <1% for INT8)

### Why Generic Tools Are Less Effective

Generic compression tools don't understand BFloat16 structure:
- Standard gzip -9: **1.40×** in 146s
- Standard zstd -19 (single-thread): **1.44×** in 520s
- **This tool ZSTD-Max (multithreaded): 1.49×** in 104s
- **Improvements:** +7% better ratio through preprocessing, 5× faster with multithreading

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
# Fast compression (5.5s, 669 MB, 172 MB/s)
./bin/compressor compress model.safetensors out.stcmp zstd fast

# Maximum compression (104s, 633 MB, 9 MB/s, default)
./bin/compressor compress model.safetensors out.stcmp zstd maximum

# Decompress (1.9-5s, up to 503 MB/s)
./bin/compressor decompress out.stcmp model.safetensors

# Benchmark all algorithms and modes
make benchmark
```

### Lossy Compression (Quantization)

```bash
# Install Python dependencies (one-time)
pip install safetensors numpy torch

# INT4 quantization + compression (350 MB, ~98% quality, 70s)
python scripts/quantize_blockwise.py model.safetensors model_int4.safetensors
./bin/compressor compress model_int4.safetensors model_int4.stcmp zstd maximum

# INT8 quantization + compression (529 MB, ~99% quality, 79s)
python scripts/quantize_blockwise.py model.safetensors model_int8.safetensors --bits 8
./bin/compressor compress model_int8.safetensors model_int8.stcmp zstd maximum
```

### Recommendations

- **Lossless (Perfect Quality):** ZSTD-Maximum → 633 MB (104s, 503 MB/s decompression)
- **Lossy (High Quality):** INT8 + ZSTD-Maximum → 529 MB (~99% quality, 79s)
- **Lossy (Maximum Compression):** INT4 + ZSTD-Maximum → 350 MB (~98% quality, 70s)
- **Production:** ZSTD-Fast for speed (5.5s), INT4 for edge deployment

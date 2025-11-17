# SafeTensors Compressor

**Information and Coding (2025/26) - Lab Work #3**
Universidade de Aveiro - DETI

Specialized compression tool for neural network weights in SafeTensors format. Implements three operation points optimized for the Qwen2-0.5B model (~1 GB).

---

## Quick Start

```bash
# Build
make build

# Run benchmarks
make benchmark

# Compress (fast/balanced/maximum)
./bin/compressor compress test/model.safetensors output/model.stcmp balanced

# Decompress
./bin/compressor decompress output/model.stcmp output/restored.safetensors
```

---

## Three Operation Points

| Mode | Preprocessing | Algorithm | Target | Use Case |
|------|--------------|-----------|--------|----------|
| **Fast** | Byte Reorder | ZSTD-10 | ~1.4x, <30s | Quick compression |
| **Balanced** | Delta Encoding | ZSTD-15 | ~2.0x, ~60s | **Best academic value** |
| **Maximum** | BF16→FP16+Delta | LZMA-9 | ~2.3x, >120s | Maximum compression |

---

## Project Structure

```
trab3/
├── bin/                # Compiled binaries (gitignored)
│   └── compressor
├── output/             # Results (gitignored)
│   ├── *.stcmp
│   ├── benchmark_results.json
│   └── benchmark_results.csv
├── includes/           # Header files
│   ├── safetensors_parser.hpp
│   ├── preprocessor.hpp
│   ├── compressor.hpp
│   └── benchmarker.hpp
├── src/                # Implementation
│   ├── main.cpp
│   ├── safetensors_parser.cpp
│   ├── preprocessor.cpp
│   ├── compressor.cpp
│   └── benchmarker.cpp
├── test/               # Test data
│   └── model.safetensors  (~943 MB, gitignored)
├── docs/               # Documentation
│   └── trab3.pdf
├── CMakeLists.txt
├── Makefile
└── README.md
```

---

## How It Works

### 1. SafeTensors Parser
Parses format: `[8B: size][JSON metadata][BFloat16 data]`
- Extracts 494M parameters
- Separates header from tensor data

### 2. Preprocessing Strategies

**Byte Reordering** (Fast)
- Separates high/low bytes of BF16 values
- Exploits pattern: 0x3c, 0xbc appear in ~28% of bytes
- `[H0 L0 H1 L1] → [H0 H1][L0 L1]`

**Delta Encoding** (Balanced)
- Stores differences between consecutive values
- Exploits local correlation in neural networks
- `[v0, v1, v2] → [v0, v1-v0, v2-v1]`

**Combined** (Maximum)
- BFloat16 → Float16 conversion (lossy)
- Then delta encoding
- Maximum entropy reduction

### 3. Compression

**ZSTD** (Fast/Balanced)
- Modern, fast compression
- Good ratios with reasonable speed
- Levels 10, 15

**LZMA** (Maximum)
- Excellent ratios, slower compression
- Used in 7-Zip, XZ
- Level 9

### 4. Information Theory

**Entropy Reduction:**
```
Original:     ~7.9 bits/byte
Byte reorder: ~7.7 bits/byte
Delta:        ~7.3 bits/byte
Combined:     ~7.0 bits/byte
```

**Why it works:**
- Neural network weights have patterns
- Preprocessing reduces entropy
- Domain-specific beats generic tools

---

## Building

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libzstd-dev liblzma-dev

# Fedora/RHEL
sudo dnf install gcc-c++ cmake zstd-devel xz-devel
```

### Build & Test
```bash
make build    # Build project
make test     # Quick integrity test
make clean    # Remove artifacts
```

---

## Usage

### Compress
```bash
# Fast mode
./bin/compressor compress test/model.safetensors output/fast.stcmp fast

# Balanced mode (default)
./bin/compressor compress test/model.safetensors output/balanced.stcmp balanced

# Maximum mode
./bin/compressor compress test/model.safetensors output/max.stcmp maximum
```

### Decompress
```bash
./bin/compressor decompress output/balanced.stcmp output/restored.safetensors
```

### Benchmark
```bash
./bin/compressor benchmark test/model.safetensors
```

**Output:**
- Console: Summary table
- `output/benchmark_results.json`: Detailed results
- `output/benchmark_results.csv`: Spreadsheet format

---

## Expected Results

### Performance

| Mode | Ratio | Size | Comp. Time | Decomp. Time |
|------|-------|------|------------|--------------|
| **Fast** | ~1.4x | ~700 MB | ~30s | ~10s |
| **Balanced** | ~2.0x | ~500 MB | ~60s | ~15s |
| **Maximum** | ~2.3x | ~430 MB | ~130s | ~20s |

### vs Standard Tools

| Tool | Ratio | Time | Notes |
|------|-------|------|-------|
| gzip -9 | 1.26x | ~75s | Baseline |
| zstd -19 | 1.3x | ~40s | Fast standard |
| xz -9 | 1.4x | ~180s | Slow standard |
| **Fast** | 1.4x | ~30s | Better + faster |
| **Balanced** | 2.0x | ~60s | **Best value** |
| **Maximum** | 2.3x | ~130s | **Best ratio** |

---

## For Academic Report

### Key Concepts Demonstrated

1. **Entropy Analysis**
   - Shannon entropy calculation
   - Entropy reduction through preprocessing
   - Theoretical compression limits

2. **Domain-Specific Optimization**
   - Understanding BFloat16 patterns
   - Exploiting neural network structure
   - Custom preprocessing beats generic tools

3. **Rate-Distortion Trade-offs**
   - Lossy vs lossless compression
   - Speed vs compression ratio
   - Three operation points show trade-offs

### Report Structure

1. **Introduction**: Problem, file analysis (BF16, 494M params)
2. **Methodology**: Format analysis, preprocessing design, operation points
3. **Implementation**: Architecture, algorithms, entropy calculations
4. **Results**: Benchmarks, comparisons, trade-off analysis
5. **Discussion**: Why preprocessing works, limitations
6. **Conclusion**: Achievements, recommendations

---

## File Format (.stcmp)

```
[5B: "STCMP"][1B: version][1B: op_point]
[8B: header_size][header][8B: data_size][compressed_data]
```

Preserves original SafeTensors metadata for perfect reconstruction.

---

## Architecture

**SafetensorsParser** - Parses SafeTensors format
**Preprocessor** - Byte reorder, delta, BF16↔FP16, entropy calc
**Compressor** - ZSTD/LZMA, custom file format
**Benchmarker** - Metrics, JSON/CSV output

---

## License

Academic project - Information and Coding
Universidade de Aveiro, 2025/26

---

## References

1. SafeTensors: https://github.com/huggingface/safetensors
2. Zstandard: https://github.com/facebook/zstd
3. LZMA: https://tukaani.org/xz/
4. BFloat16: https://en.wikipedia.org/wiki/Bfloat16_floating-point_format

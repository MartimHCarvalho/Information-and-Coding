#!/usr/bin/env python3
"""
SafeTensors File Analysis Tool
Analyzes file structure, byte patterns, and entropy for academic reporting
"""

import struct
import json
import sys
from collections import Counter
import math

def calculate_entropy(data):
    """Calculate Shannon entropy of byte sequence"""
    if not data:
        return 0.0

    counter = Counter(data)
    length = len(data)
    entropy = 0.0

    for count in counter.values():
        probability = count / length
        entropy -= probability * math.log2(probability)

    return entropy

def analyze_safetensors(filepath):
    """Deep analysis of SafeTensors file structure"""

    print("=" * 80)
    print("SAFETENSORS FILE STRUCTURE ANALYSIS")
    print("=" * 80)
    print()

    with open(filepath, 'rb') as f:
        # Read header size (first 8 bytes, little-endian)
        header_size_bytes = f.read(8)
        header_size = struct.unpack('<Q', header_size_bytes)[0]

        print(f"📄 FILE: {filepath}")
        print(f"   Total size: {f.seek(0, 2) / (1024**3):.2f} GB")
        f.seek(8)

        print()
        print("-" * 80)
        print("1. HEADER STRUCTURE")
        print("-" * 80)

        # Read header (JSON metadata)
        header_json = f.read(header_size).decode('utf-8')
        header_data = json.loads(header_json)

        print(f"   Header size: {header_size:,} bytes ({header_size / 1024:.2f} KB)")
        print(f"   Format: JSON (UTF-8 encoded)")
        print()

        # Analyze metadata
        print("   Metadata contents:")
        tensor_count = len([k for k in header_data.keys() if k != '__metadata__'])
        print(f"   - Number of tensors: {tensor_count}")

        if '__metadata__' in header_data:
            print(f"   - Metadata fields: {list(header_data['__metadata__'].keys())}")

        print()
        print("   Tensor information:")

        total_params = 0
        dtypes = Counter()
        shapes_info = []

        for name, info in header_data.items():
            if name == '__metadata__':
                continue

            dtype = info['dtype']
            shape = info['shape']
            data_offsets = info['data_offsets']

            dtypes[dtype] += 1

            # Calculate number of parameters
            params = 1
            for dim in shape:
                params *= dim
            total_params += params

            tensor_size = data_offsets[1] - data_offsets[0]
            shapes_info.append({
                'name': name,
                'dtype': dtype,
                'shape': shape,
                'params': params,
                'size_bytes': tensor_size
            })

        print(f"   - Total parameters: {total_params:,}")
        print(f"   - Data types: {dict(dtypes)}")
        print()

        # Show sample tensors
        print("   Sample tensor structures (first 5):")
        for i, tensor in enumerate(sorted(shapes_info, key=lambda x: x['size_bytes'], reverse=True)[:5]):
            print(f"     {i+1}. {tensor['name']}")
            print(f"        Shape: {tensor['shape']}")
            print(f"        Parameters: {tensor['params']:,}")
            print(f"        Size: {tensor['size_bytes'] / (1024**2):.2f} MB")

        print()
        print("-" * 80)
        print("2. TENSOR DATA ANALYSIS")
        print("-" * 80)

        # Read all tensor data
        data_start = 8 + header_size
        f.seek(data_start)
        tensor_data = f.read()

        data_size_mb = len(tensor_data) / (1024**2)
        print(f"   Total tensor data: {data_size_mb:.2f} MB")
        print(f"   Format: BFloat16 (16-bit floating point)")
        print(f"   Number of BF16 values: {len(tensor_data) // 2:,}")
        print()

        # Analyze byte patterns
        print("-" * 80)
        print("3. BYTE-LEVEL ANALYSIS")
        print("-" * 80)

        # Analyze full file (set to 10 MB for faster analysis if needed)
        # sample_size = min(10 * 1024 * 1024, len(tensor_data))  # Fast mode
        sample_size = len(tensor_data)  # Full analysis
        sample_data = tensor_data[:sample_size]

        print(f"Analyzing {'FULL FILE' if sample_size == len(tensor_data) else 'sample'}: {sample_size / (1024**2):.1f} MB")
        print()

        # Overall byte frequency
        byte_freq = Counter(sample_data)
        total_bytes = len(sample_data)

        print("   Top 10 most frequent bytes:")
        for byte_val, count in byte_freq.most_common(10):
            percentage = (count / total_bytes) * 100
            print(f"     0x{byte_val:02x}: {count:,} occurrences ({percentage:.2f}%)")

        print()

        # Analyze high bytes vs low bytes separately
        high_bytes = sample_data[0::2]  # Even positions
        low_bytes = sample_data[1::2]   # Odd positions

        high_freq = Counter(high_bytes)
        low_freq = Counter(low_bytes)

        print("   High bytes (BF16 exponent + sign):")
        print("   Top 5 most frequent:")
        for byte_val, count in high_freq.most_common(5):
            percentage = (count / len(high_bytes)) * 100
            print(f"     0x{byte_val:02x}: {percentage:.2f}%")

        print()
        print("   Low bytes (BF16 mantissa):")
        print("   Top 5 most frequent:")
        for byte_val, count in low_freq.most_common(5):
            percentage = (count / len(low_bytes)) * 100
            print(f"     0x{byte_val:02x}: {percentage:.2f}%")

        print()
        print("-" * 80)
        print("4. ENTROPY ANALYSIS")
        print("-" * 80)

        # Calculate entropies
        overall_entropy = calculate_entropy(sample_data)
        high_entropy = calculate_entropy(high_bytes)
        low_entropy = calculate_entropy(low_bytes)

        print(f"   Overall entropy: {overall_entropy:.4f} bits/byte")
        print(f"   High bytes entropy: {high_entropy:.4f} bits/byte")
        print(f"   Low bytes entropy: {low_entropy:.4f} bits/byte")
        print()
        print(f"   Theoretical maximum: 8.000 bits/byte (random data)")
        print(f"   Compression potential: {(8.0 - overall_entropy) / 8.0 * 100:.1f}%")

        print()
        print("-" * 80)
        print("5. BFLOAT16 VALUE ANALYSIS")
        print("-" * 80)

        # Analyze BF16 values
        num_values = len(sample_data) // 2

        # Count zeros, positives, negatives
        zeros = 0
        positives = 0
        negatives = 0
        small_values = 0  # |value| < 0.1

        for i in range(0, len(sample_data), 2):
            bf16_bytes = sample_data[i:i+2]
            bf16_val = struct.unpack('<H', bf16_bytes)[0]

            # Check sign bit (bit 15)
            if bf16_val & 0x8000:
                negatives += 1
            elif bf16_val == 0:
                zeros += 1
            else:
                positives += 1

            # Check if small value (exponent < 124)
            exponent = (bf16_val >> 7) & 0xFF
            if exponent < 124:
                small_values += 1

        print(f"   Total values analyzed: {num_values:,}")
        print(f"   Zeros: {zeros:,} ({zeros/num_values*100:.2f}%)")
        print(f"   Positive values: {positives:,} ({positives/num_values*100:.2f}%)")
        print(f"   Negative values: {negatives:,} ({negatives/num_values*100:.2f}%)")
        print(f"   Small values (|x| < 0.1): {small_values:,} ({small_values/num_values*100:.2f}%)")

        print()
        print("-" * 80)
        print("6. BYTE REORDERING IMPACT SIMULATION")
        print("-" * 80)

        # Simulate byte reordering
        reordered = bytearray(len(sample_data))
        half = len(sample_data) // 2

        # Separate high and low bytes
        reordered[:half] = sample_data[0::2]  # High bytes
        reordered[half:] = sample_data[1::2]  # Low bytes

        reordered_entropy = calculate_entropy(reordered)
        entropy_reduction = overall_entropy - reordered_entropy

        print(f"   Original entropy: {overall_entropy:.4f} bits/byte")
        print(f"   After byte reordering: {reordered_entropy:.4f} bits/byte")
        print(f"   Entropy reduction: {entropy_reduction:.4f} bits/byte ({entropy_reduction/overall_entropy*100:.1f}%)")
        print()

        if entropy_reduction > 0:
            print("   ✅ Byte reordering REDUCES entropy (good for compression)")
        else:
            print("   ❌ Byte reordering INCREASES entropy (bad for compression)")

        print()
        print("-" * 80)
        print("7. COMPRESSIBILITY ESTIMATION")
        print("-" * 80)

        # Estimate theoretical compression limits
        theoretical_min_bits = overall_entropy * len(tensor_data)
        theoretical_min_bytes = theoretical_min_bits / 8
        theoretical_ratio = len(tensor_data) / theoretical_min_bytes

        print(f"   Shannon entropy limit:")
        print(f"   - Minimum size: {theoretical_min_bytes / (1024**2):.2f} MB")
        print(f"   - Maximum ratio: {theoretical_ratio:.2f}x")
        print()
        print(f"   After preprocessing:")
        reordered_min_bits = reordered_entropy * len(tensor_data)
        reordered_min_bytes = reordered_min_bits / 8
        reordered_ratio = len(tensor_data) / reordered_min_bytes
        print(f"   - Minimum size: {reordered_min_bytes / (1024**2):.2f} MB")
        print(f"   - Maximum ratio: {reordered_ratio:.2f}x")
        print()
        print("   Note: Real compressors achieve 60-70% of theoretical maximum")
        print(f"   Expected practical ratio: {reordered_ratio * 0.65:.2f}x")

        print()
        print("=" * 80)
        print("ANALYSIS COMPLETE")
        print("=" * 80)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python3 safetensors_analysis.py <model.safetensors>")
        sys.exit(1)

    analyze_safetensors(sys.argv[1])

import sys
import argparse
import numpy as np
from pathlib import Path

try:
    from safetensors import safe_open
    from safetensors.numpy import save_file
except ImportError:
    print("Error: safetensors package not found!")
    print("Install with: pip install safetensors")
    sys.exit(1)

try:
    import torch
    TORCH_AVAILABLE = True
except ImportError:
    print("Warning: PyTorch not found. BFloat16 models may not load correctly.")
    print("Install with: pip install torch")
    TORCH_AVAILABLE = False


def quantize_blockwise(tensor, block_size=128, bits=4):
    if bits not in [4, 8]:
        raise ValueError(f"Unsupported bit width: {bits}. Must be 4 or 8.")

    original_shape = tensor.shape
    original_dtype = tensor.dtype

    # Convert to float32 
    if original_dtype != np.float32:
        tensor = tensor.astype(np.float32)

    flat = tensor.flatten()
    n_elements = len(flat)

    n_blocks = (n_elements + block_size - 1) // block_size
    n_pad = n_blocks * block_size - n_elements

    if n_pad > 0:
        flat = np.pad(flat, (0, n_pad), mode='constant', constant_values=0)

    blocks = flat.reshape(n_blocks, block_size)

    if bits == 4:
        max_val = 7      # INT4 range: -8 to 7
        min_val = -8
        dtype = np.int8  
    else:  
        max_val = 127    # INT8 range: -128 to 127
        min_val = -128
        dtype = np.int8

    quantized_blocks = np.zeros((n_blocks, block_size), dtype=dtype)
    scales = np.zeros(n_blocks, dtype=np.float32)

    for i in range(n_blocks):
        block = blocks[i]

        max_abs = np.abs(block).max()

        if max_abs > 0:
            scale = max_abs / max_val
        else:
            scale = 1.0  

        quantized = np.round(block / scale).clip(min_val, max_val).astype(dtype)

        quantized_blocks[i] = quantized
        scales[i] = scale

    quantized_flat = quantized_blocks.flatten()[:n_elements]

    if bits == 4:
        if len(quantized_flat) % 2 == 1:
            quantized_flat = np.append(quantized_flat, 0)

        packed = np.zeros(len(quantized_flat) // 2, dtype=np.uint8)
        for i in range(len(packed)):
            val1 = int(quantized_flat[2*i]) & 0x0F      
            val2 = int(quantized_flat[2*i + 1]) & 0x0F  
            packed[i] = ((val1 << 4) | val2)            

        quantized_tensor = packed
    else:
        quantized_tensor = quantized_flat.reshape(original_shape)

    return quantized_tensor, scales, original_shape, bits


def should_quantize_tensor(name, tensor):
    name_lower = name.lower()

    if tensor.size < 512:
        return False

    if 'embed' in name_lower:
        return False

    if 'norm' in name_lower or 'ln' in name_lower:
        return False

    if 'bias' in name_lower:
        return False

    if 'weight' in name_lower:
        return True

    if tensor.size > 10000:
        return True

    return False


def quantize_model(input_path, output_path, block_size=128, bits=4, verbose=True):
    if verbose:
        print(f"Loading model from: {input_path}")

    tensors = {}
    framework = "pt" if TORCH_AVAILABLE else "numpy"

    with safe_open(input_path, framework=framework) as f:
        tensor_keys = f.keys()
        if verbose:
            print(f"Found {len(list(tensor_keys))} tensors")

        for key in f.keys():
            tensor = f.get_tensor(key)

            if TORCH_AVAILABLE and hasattr(tensor, 'numpy'):
                if tensor.dtype == torch.bfloat16:
                    tensor = tensor.float().numpy()
                else:
                    tensor = tensor.numpy()
            elif hasattr(tensor, 'dtype') and tensor.dtype == np.uint16:
                pass
            tensors[key] = tensor

    stats = {
        'total_tensors': len(tensors),
        'quantized_tensors': 0,
        'kept_original': 0,
        'original_size_mb': 0,
        'quantized_size_mb': 0,
        'scales_size_mb': 0
    }

    quantized_tensors = {}

    for name, tensor in tensors.items():
        original_size = tensor.nbytes
        stats['original_size_mb'] += original_size / (1024 * 1024)

        if should_quantize_tensor(name, tensor):
            if verbose:
                print(f"Quantizing: {name:60s} shape={str(tensor.shape):20s} dtype={str(tensor.dtype):10s}", end="")

            q_tensor, scales, orig_shape, used_bits = quantize_blockwise(tensor, block_size, bits)

            quantized_tensors[name] = q_tensor
            quantized_tensors[name + ".__scales__"] = scales

            stats['quantized_tensors'] += 1
            stats['quantized_size_mb'] += q_tensor.nbytes / (1024 * 1024)
            stats['scales_size_mb'] += scales.nbytes / (1024 * 1024)

            if verbose:
                reduction = (1 - q_tensor.nbytes / original_size) * 100
                print(f" → INT{used_bits} (reduced {reduction:.1f}%)")
        else:
            quantized_tensors[name] = tensor
            stats['kept_original'] += 1
            stats['quantized_size_mb'] += original_size / (1024 * 1024)

            if verbose:
                print(f"Keeping:    {name:60s} shape={str(tensor.shape):20s} dtype={str(tensor.dtype):10s} (not quantized)")

    if verbose:
        print(f"\nSaving quantized model to: {output_path}")

    save_file(quantized_tensors, output_path)

    total_quantized = stats['quantized_size_mb'] + stats['scales_size_mb']
    compression_ratio = stats['original_size_mb'] / total_quantized
    size_reduction = (1 - total_quantized / stats['original_size_mb']) * 100

    stats['total_size_mb'] = total_quantized
    stats['compression_ratio'] = compression_ratio
    stats['size_reduction_percent'] = size_reduction

    return stats


def print_statistics(stats, bits=4):
    print("\n" + "="*70)
    print(f"QUANTIZATION COMPLETE (INT{bits})")
    print("="*70)
    print(f"Total tensors:           {stats['total_tensors']}")
    print(f"  Quantized to INT{bits}:       {stats['quantized_tensors']}")
    print(f"  Kept original:         {stats['kept_original']}")
    print()
    print(f"Original size:           {stats['original_size_mb']:.2f} MB (BFloat16/Float32)")
    print(f"Quantized weights:       {stats['quantized_size_mb']:.2f} MB (INT{bits} packed)")
    print(f"Scale factors:           {stats['scales_size_mb']:.2f} MB")
    print(f"Total quantized size:    {stats['total_size_mb']:.2f} MB")
    print()
    print(f"Compression ratio:       {stats['compression_ratio']:.2f}×")
    print(f"Size reduction:          {stats['size_reduction_percent']:.1f}%")
    print("="*70)
    print()
    print("THIS IS STEP 1 OF 2!")
    print()
    print("Next: Compress the quantized file with ZSTD (Step 2/2)")
    print(f"  ./bin/compressor compress <quantized_file> <output.stcmp> zstd maximum")
    print()
    expected_final = 350 if bits == 4 else 529
    print(f"Expected final result after ZSTD compression:")
    print(f"  ~{expected_final} MB (INT{bits} + ZSTD)")
    print()


def main():
    parser = argparse.ArgumentParser(
        description="Block-wise INT4/INT8 quantization for neural network weights",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Basic usage with INT4 (recommended, block size = 128)
  python scripts/quantize_blockwise.py test/model.safetensors test/model_int4.safetensors

  # Use INT8 instead of INT4
  python scripts/quantize_blockwise.py input.safetensors output.safetensors --bits 8

  # Custom block size
  python scripts/quantize_blockwise.py input.safetensors output.safetensors --block-size 256

  # Quiet mode
  python scripts/quantize_blockwise.py input.safetensors output.safetensors --quiet

    This is a 2-step process!
  Step 1: Run this script to quantize (BFloat16 → INT4/INT8)
  Step 2: Compress the quantized file with ZSTD

After quantization (Step 1), compress with ZSTD (Step 2):
  ./bin/compressor compress <quantized_file> output.stcmp zstd maximum

Expected results (Qwen2-0.5B, 943 MB model):
  Step 1 (Quantization):
    INT4: 943 MB → ~701 MB (INT4 quantized model)
    INT8: 943 MB → ~872 MB (INT8 quantized model)

  Step 2 (After ZSTD compression):
    INT4 + ZSTD: ~350 MB final (2.69× total compression)
    INT8 + ZSTD: ~529 MB final (1.78× total compression)
        """
    )

    parser.add_argument("input", help="Input SafeTensors file (BFloat16/Float32)")
    parser.add_argument("output", help="Output SafeTensors file (quantized + scales)")
    parser.add_argument("--bits", type=int, default=4, choices=[4, 8],
                       help="Quantization bit width: 4 (INT4, default) or 8 (INT8)")
    parser.add_argument("--block-size", type=int, default=128,
                       help="Block size for quantization (default: 128)")
    parser.add_argument("--quiet", action="store_true",
                       help="Suppress progress output")

    args = parser.parse_args()

    input_path = Path(args.input)
    if not input_path.exists():
        print(f"Error: Input file not found: {input_path}")
        sys.exit(1)

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    try:
        stats = quantize_model(
            str(input_path),
            str(output_path),
            block_size=args.block_size,
            bits=args.bits,
            verbose=not args.quiet
        )

        if not args.quiet:
            print_statistics(stats, bits=args.bits)
            print(f"✓ Success! Quantized model saved to: {output_path}")

    except Exception as e:
        print(f"Error during quantization: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
Enhanced Image Codec Performance Visualizer
Parses JSON results or test output and creates comprehensive performance graphs
"""

import re
import matplotlib.pyplot as plt
import numpy as np
from collections import defaultdict
import sys
import json
import os

def parse_json_results(filename):
    """Parse JSON results file from test output"""
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        stats = {}
        for entry in data:
            predictor = entry.get('test_name', entry.get('predictor', 'Unknown'))
            stats[predictor] = {
                'compression_ratio': entry['compression_ratio'],
                'bits_per_pixel': entry['bits_per_pixel'],
                'space_savings': entry['space_savings'],
                'optimal_m': entry['optimal_m'],
                'encode_time': entry['encode_time'],
                'decode_time': entry['decode_time'],
                'original_size': entry.get('original_size', 0),
                'compressed_size': entry.get('compressed_size', 0),
                'reconstruction_perfect': entry.get('reconstruction_perfect', True)
            }
        
        print(f"✅ Successfully parsed JSON file: {filename}")
        print(f"   Found {len(stats)} test results")
        return stats
        
    except json.JSONDecodeError as e:
        print(f"❌ JSON parsing error: {e}")
        return None
    except Exception as e:
        print(f"❌ Error reading JSON file: {e}")
        return None

def parse_test_output(filename):
    """Parse the test output text file and extract statistics"""
    stats = defaultdict(dict)
    
    # Try different encodings to handle special characters
    encodings = ['utf-8', 'latin-1', 'cp1252', 'iso-8859-1']
    content = None
    
    for encoding in encodings:
        try:
            with open(filename, 'r', encoding=encoding, errors='ignore') as f:
                content = f.read()
            break
        except:
            continue
    
    if content is None:
        print(f"❌ Could not read file: {filename}")
        return stats
    
    # Find all test sections
    pattern = r'Image Test: (\w+).*?Compression ratio:\s+([\d.]+):1.*?Bits per pixel:\s+([\d.]+).*?Space savings:\s+([\d.]+)%.*?Optimal m:\s+(\d+).*?Encoding time:\s+(\d+) ms.*?Decoding time:\s+(\d+) ms'
    
    matches = re.findall(pattern, content, re.DOTALL)
    
    for match in matches:
        predictor = match[0]
        stats[predictor] = {
            'compression_ratio': float(match[1]),
            'bits_per_pixel': float(match[2]),
            'space_savings': float(match[3]),
            'optimal_m': int(match[4]),
            'encode_time': int(match[5]),
            'decode_time': int(match[6])
        }
    
    if stats:
        print(f"✅ Successfully parsed text output: {filename}")
        print(f"   Found {len(stats)} test results")
    
    return stats

def calculate_velocities(stats):
    """Calculate compression and decompression velocities (MB/s)"""
    # Calculate image size from original_size if available
    test_image_mb = 0.0625  # Default: 256x256 = 65536 bytes ≈ 0.0625 MB
    
    # Try to get actual size from first entry
    for predictor in stats:
        if 'original_size' in stats[predictor] and stats[predictor]['original_size'] > 0:
            test_image_mb = stats[predictor]['original_size'] / (1024 * 1024)
            break
    
    for predictor in stats:
        encode_time_s = stats[predictor]['encode_time'] / 1000.0
        decode_time_s = stats[predictor]['decode_time'] / 1000.0
        
        # Velocity = size / time (MB/s)
        stats[predictor]['encode_velocity'] = test_image_mb / encode_time_s if encode_time_s > 0 else 0
        stats[predictor]['decode_velocity'] = test_image_mb / decode_time_s if decode_time_s > 0 else 0
    
    return stats

def create_enhanced_visualizations(stats, output_filename='codec_performance_enhanced.png'):
    """Create comprehensive performance graphs"""
    
    if not stats:
        print("❌ No data to visualize.")
        return
    
    # Calculate velocities
    stats = calculate_velocities(stats)
    
    predictors = list(stats.keys())
    
    # Set up the style
    try:
        plt.style.use('seaborn-v0_8-darkgrid')
    except:
        try:
            plt.style.use('seaborn-darkgrid')
        except:
            pass  # Use default style
    
    # Create figure with 3x2 layout for 6 graphs
    fig = plt.figure(figsize=(18, 12))
    gs = fig.add_gridspec(3, 2, hspace=0.45, wspace=0.35)
    
    fig.suptitle('Image Codec Performance Analysis - Golomb Coding', 
                 fontsize=18, fontweight='bold', y=0.98)
    
    # Color palette
    colors = {
        'primary': '#2ecc71',
        'secondary': '#3498db',
        'tertiary': '#e74c3c',
        'quaternary': '#9b59b6',
        'quinary': '#f39c12',
        'senary': '#1abc9c'
    }
    
    # 1. Compression Ratio (Top Left)
    ax1 = fig.add_subplot(gs[0, 0])
    compression_ratios = [stats[p]['compression_ratio'] for p in predictors]
    bars1 = ax1.bar(predictors, compression_ratios, color=colors['primary'], 
                    alpha=0.8, edgecolor='black', linewidth=1.5)
    ax1.set_title('Compression Ratio', fontweight='bold', fontsize=12, pad=10)
    ax1.set_ylabel('Ratio (higher is better)', fontweight='bold')
    ax1.set_xlabel('Predictor Type', fontweight='bold')
    ax1.grid(True, alpha=0.3, axis='y')
    ax1.axhline(y=1.0, color='r', linestyle='--', alpha=0.5, linewidth=2, label='No compression')
    ax1.legend(loc='upper right')
    
    # Add value labels on bars
    for bar, val in zip(bars1, compression_ratios):
        height = bar.get_height()
        ax1.text(bar.get_x() + bar.get_width()/2., height,
                f'{val:.2f}:1', ha='center', va='bottom', fontweight='bold', fontsize=9)
    ax1.tick_params(axis='x', rotation=45)
    
    # 2. Bits per Pixel (Top Right)
    ax2 = fig.add_subplot(gs[0, 1])
    bits_per_pixel = [stats[p]['bits_per_pixel'] for p in predictors]
    bars2 = ax2.bar(predictors, bits_per_pixel, color=colors['secondary'], 
                    alpha=0.8, edgecolor='black', linewidth=1.5)
    ax2.set_title('Bits per Pixel', fontweight='bold', fontsize=12, pad=10)
    ax2.set_ylabel('Bits (lower is better)', fontweight='bold')
    ax2.set_xlabel('Predictor Type', fontweight='bold')
    ax2.grid(True, alpha=0.3, axis='y')
    ax2.axhline(y=8.0, color='r', linestyle='--', alpha=0.5, linewidth=2, label='Original (8 bpp)')
    ax2.legend(loc='upper right')
    
    # Add value labels on bars
    for bar, val in zip(bars2, bits_per_pixel):
        height = bar.get_height()
        ax2.text(bar.get_x() + bar.get_width()/2., height,
                f'{val:.2f}', ha='center', va='bottom', fontweight='bold', fontsize=9)
    ax2.tick_params(axis='x', rotation=45)
    
    # 3. Compression/Decompression Velocity (Middle Left)
    ax3 = fig.add_subplot(gs[1, 0])
    encode_velocities = [stats[p]['encode_velocity'] for p in predictors]
    decode_velocities = [stats[p]['decode_velocity'] for p in predictors]
    
    x_pos = np.arange(len(predictors))
    width = 0.35
    
    bars3a = ax3.bar(x_pos - width/2, encode_velocities, width, 
                     label='Compression', color=colors['quinary'], alpha=0.8, 
                     edgecolor='black', linewidth=1.5)
    bars3b = ax3.bar(x_pos + width/2, decode_velocities, width, 
                     label='Decompression', color=colors['senary'], alpha=0.8, 
                     edgecolor='black', linewidth=1.5)
    
    ax3.set_title('Compression/Decompression Throughput', fontweight='bold', fontsize=12, pad=10)
    ax3.set_ylabel('Throughput (MB/s, higher is better)', fontweight='bold')
    ax3.set_xlabel('Predictor Type', fontweight='bold')
    ax3.set_xticks(x_pos)
    ax3.set_xticklabels(predictors, rotation=45, ha='right')
    ax3.legend(loc='upper right')
    ax3.grid(True, alpha=0.3, axis='y')
    
    # Add value labels
    for bar in bars3a:
        height = bar.get_height()
        if height > 0:
            ax3.text(bar.get_x() + bar.get_width()/2., height,
                    f'{height:.2f}', ha='center', va='bottom', fontweight='bold', fontsize=8)
    for bar in bars3b:
        height = bar.get_height()
        if height > 0:
            ax3.text(bar.get_x() + bar.get_width()/2., height,
                    f'{height:.2f}', ha='center', va='bottom', fontweight='bold', fontsize=8)
    
    # 4. Optimal m Parameter (Middle Right)
    ax4 = fig.add_subplot(gs[1, 1])
    optimal_m = [stats[p]['optimal_m'] for p in predictors]
    bars4 = ax4.bar(predictors, optimal_m, color=colors['quaternary'], 
                    alpha=0.8, edgecolor='black', linewidth=1.5)
    ax4.set_title('Optimal m Parameter (Golomb)', fontweight='bold', fontsize=12, pad=10)
    ax4.set_ylabel('m value', fontweight='bold')
    ax4.set_xlabel('Predictor Type', fontweight='bold')
    ax4.grid(True, alpha=0.3, axis='y')
    
    # Add value labels and color coding based on m value
    for bar, val in zip(bars4, optimal_m):
        height = bar.get_height()
        ax4.text(bar.get_x() + bar.get_width()/2., height,
                str(val), ha='center', va='bottom', fontweight='bold', fontsize=10)
        
        # Color code based on m value range
        if val < 10:
            bar.set_color('#e74c3c')  # Red for low m (high compression)
        elif val < 20:
            bar.set_color('#f39c12')  # Orange for medium m
        else:
            bar.set_color('#9b59b6')  # Purple for high m (lower compression)
    
    ax4.tick_params(axis='x', rotation=45)
    
    # Add legend for m value ranges
    from matplotlib.patches import Patch
    legend_elements = [
        Patch(facecolor='#e74c3c', label='Low m (<10): High compression'),
        Patch(facecolor='#f39c12', label='Medium m (10-20): Balanced'),
        Patch(facecolor='#9b59b6', label='High m (>20): Lower compression')
    ]
    ax4.legend(handles=legend_elements, loc='upper right', fontsize=8)
    
    # 5. Space Savings (Bottom Left)
    ax5 = fig.add_subplot(gs[2, 0])
    space_savings = [stats[p]['space_savings'] for p in predictors]
    bars5 = ax5.bar(predictors, space_savings, color=colors['tertiary'], 
                    alpha=0.8, edgecolor='black', linewidth=1.5)
    ax5.set_title('Space Savings', fontweight='bold', fontsize=12, pad=10)
    ax5.set_ylabel('Percentage (%)', fontweight='bold')
    ax5.set_xlabel('Predictor Type', fontweight='bold')
    ax5.grid(True, alpha=0.3, axis='y')
    ax5.set_ylim(0, 100)
    
    # Add value labels on bars
    for bar, val in zip(bars5, space_savings):
        height = bar.get_height()
        ax5.text(bar.get_x() + bar.get_width()/2., height,
                f'{val:.1f}%', ha='center', va='bottom', fontweight='bold', fontsize=9)
    ax5.tick_params(axis='x', rotation=45)
    
    # 6. Processing Time Comparison (Bottom Right)
    ax6 = fig.add_subplot(gs[2, 1])
    encode_times = [stats[p]['encode_time'] for p in predictors]
    decode_times = [stats[p]['decode_time'] for p in predictors]
    
    x_pos = np.arange(len(predictors))
    width = 0.35
    
    bars6a = ax6.bar(x_pos - width/2, encode_times, width, 
                     label='Encoding', color=colors['quinary'], alpha=0.8, 
                     edgecolor='black', linewidth=1.5)
    bars6b = ax6.bar(x_pos + width/2, decode_times, width, 
                     label='Decoding', color=colors['senary'], alpha=0.8, 
                     edgecolor='black', linewidth=1.5)
    
    ax6.set_title('Processing Time', fontweight='bold', fontsize=12, pad=10)
    ax6.set_ylabel('Time (ms, lower is better)', fontweight='bold')
    ax6.set_xlabel('Predictor Type', fontweight='bold')
    ax6.set_xticks(x_pos)
    ax6.set_xticklabels(predictors, rotation=45, ha='right')
    ax6.legend(loc='upper right')
    ax6.grid(True, alpha=0.3, axis='y')
    
    # Add value labels
    for bar in bars6a:
        height = bar.get_height()
        ax6.text(bar.get_x() + bar.get_width()/2., height,
                f'{int(height)}', ha='center', va='bottom', fontweight='bold', fontsize=8)
    for bar in bars6b:
        height = bar.get_height()
        ax6.text(bar.get_x() + bar.get_width()/2., height,
                f'{int(height)}', ha='center', va='bottom', fontweight='bold', fontsize=8)
    
    # Save figure
    plt.savefig(output_filename, dpi=300, bbox_inches='tight')
    print(f"✅ Saved: {output_filename}")
    plt.show()

def create_summary_report(stats):
    """Create a text summary report"""
    
    if not stats:
        return
    
    print("\n" + "="*80)
    print(" " * 25 + "PERFORMANCE SUMMARY REPORT")
    print("="*80)
    
    # Find best performers
    best_ratio = max(stats.items(), key=lambda x: x[1]['compression_ratio'])
    best_speed = max(stats.items(), key=lambda x: x[1]['encode_velocity'])
    lowest_m = min(stats.items(), key=lambda x: x[1]['optimal_m'])
    best_bpp = min(stats.items(), key=lambda x: x[1]['bits_per_pixel'])
    
    print(f"\n🏆 Best Compression Ratio: {best_ratio[0]}")
    print(f"   └─ {best_ratio[1]['compression_ratio']:.3f}:1 ({best_ratio[1]['space_savings']:.1f}% savings)")
    print(f"   └─ {best_ratio[1]['bits_per_pixel']:.3f} bits/pixel")
    
    print(f"\n⚡ Fastest Compression: {best_speed[0]}")
    print(f"   └─ {best_speed[1]['encode_velocity']:.3f} MB/s encoding")
    print(f"   └─ {best_speed[1]['decode_velocity']:.3f} MB/s decoding")
    
    print(f"\n🎯 Most Efficient Predictor (lowest m): {lowest_m[0]}")
    print(f"   └─ m = {lowest_m[1]['optimal_m']} (lower m = better prediction)")
    
    print(f"\n📉 Lowest Bits/Pixel: {best_bpp[0]}")
    print(f"   └─ {best_bpp[1]['bits_per_pixel']:.3f} bits/pixel (vs 8.0 original)")
    
    print("\n" + "-"*80)
    print(" " * 30 + "DETAILED METRICS")
    print("-"*80)
    print(f"{'Predictor':<12} {'Ratio':<12} {'BPP':<10} {'m':<8} {'Enc(MB/s)':<12} {'Dec(MB/s)':<12}")
    print("-"*80)
    
    # Sort by compression ratio (best first)
    sorted_predictors = sorted(stats.items(), key=lambda x: x[1]['compression_ratio'], reverse=True)
    
    for predictor, s in sorted_predictors:
        perfect = "✓" if s.get('reconstruction_perfect', True) else "✗"
        print(f"{predictor:<12} {s['compression_ratio']:>6.3f}:1     {s['bits_per_pixel']:>6.3f}    "
              f"{s['optimal_m']:>4}     {s['encode_velocity']:>8.3f}      {s['decode_velocity']:>8.3f}  {perfect}")
    
    print("="*80)
    
    # Additional statistics
    avg_ratio = sum(s['compression_ratio'] for s in stats.values()) / len(stats)
    avg_bpp = sum(s['bits_per_pixel'] for s in stats.values()) / len(stats)
    avg_savings = sum(s['space_savings'] for s in stats.values()) / len(stats)
    
    print(f"\n📊 Average Performance:")
    print(f"   ├─ Compression Ratio: {avg_ratio:.3f}:1")
    print(f"   ├─ Bits per Pixel: {avg_bpp:.3f}")
    print(f"   └─ Space Savings: {avg_savings:.1f}%")
    print("\n")

def auto_detect_file_type(filename):
    """Automatically detect if file is JSON or text output"""
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            first_char = f.read(1)
            if first_char == '[' or first_char == '{':
                return 'json'
            else:
                return 'text'
    except:
        return None

def main():
    """Main function"""
    print("=" * 80)
    print(" " * 20 + "Image Codec Performance Visualizer")
    print(" " * 25 + "Golomb Coding Analysis")
    print("=" * 80)
    
    stats = None
    
    if len(sys.argv) > 1:
        filename = sys.argv[1]
        
        if not os.path.exists(filename):
            print(f"\n❌ Error: File not found: {filename}")
            print("\nUsage: python visualizerImage.py <input_file>")
            print("  Supported formats:")
            print("    - JSON results file (image_codec_results.json)")
            print("    - Text output file from test program")
            return
        
        print(f"\n📂 Input file: {filename}")
        
        # Auto-detect file type
        file_type = auto_detect_file_type(filename)
        
        if file_type == 'json':
            print("   Detected format: JSON")
            stats = parse_json_results(filename)
        elif file_type == 'text':
            print("   Detected format: Text output")
            stats = parse_test_output(filename)
        else:
            print("   Unable to detect format, trying both parsers...")
            stats = parse_json_results(filename)
            if not stats:
                stats = parse_test_output(filename)
        
    else:
        # Try default filenames
        default_files = ['image_codec_results.json', 'test_output.txt']
        
        for default_file in default_files:
            if os.path.exists(default_file):
                print(f"\n📂 Found default file: {default_file}")
                if default_file.endswith('.json'):
                    stats = parse_json_results(default_file)
                else:
                    stats = parse_test_output(default_file)
                if stats:
                    break
        
        if not stats:
            print("\n⚠️  No input file provided and no default files found.")
            print("\nUsage: python visualizerImage.py <input_file>")
            print("\nSupported input files:")
            print("  • image_codec_results.json (JSON format)")
            print("  • test_output.txt (captured program output)")
            print("\nTo capture test output:")
            print("  make run-image FILE=./images/airplane.pgm > test_output.txt")
            print("  python visualizerImage.py test_output.txt")
            print("\nOr if JSON results exist:")
            print("  python visualizerImage.py image_codec_results.json")
            return
    
    if not stats:
        print("\n❌ No data available to visualize.")
        print("   Check that the input file contains valid test results.")
        return
    
    print(f"\n✅ Successfully loaded {len(stats)} test results:")
    for predictor in stats:
        print(f"   • {predictor}")
    
    # Create visualizations
    print("\n📈 Generating performance graphs...")
    create_enhanced_visualizations(stats)
    
    # Create text summary
    create_summary_report(stats)
    
    print("✅ Analysis complete!")
    print("\n📁 Output files:")
    print("   • codec_performance_enhanced.png - Comprehensive 6-panel visualization")

if __name__ == "__main__":
    main()
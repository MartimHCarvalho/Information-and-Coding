#!/usr/bin/env python3
"""
Enhanced Audio Codec Performance Visualizer
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
            
            # Handle both old and new JSON formats
            # Try new format first (optimal_m_ch1/ch2), then fall back to old format (optimal_m)
            if 'optimal_m_ch1' in entry:
                m1 = entry['optimal_m_ch1']
                m2 = entry.get('optimal_m_ch2', m1)
            else:
                # Old format - use single optimal_m for both channels
                m1 = entry.get('optimal_m', 8)  # Default to 8 if missing
                m2 = m1
            
            # Handle both time key formats
            encode_time = entry.get('encode_time_ms', entry.get('encodeTimeMs', entry.get('encode_time', 0)))
            decode_time = entry.get('decode_time_ms', entry.get('decodeTimeMs', entry.get('decode_time', 0)))

            stats[predictor] = {
                'compression_ratio': entry.get('compression_ratio', entry.get('compressionRatio', 1.0)),
                'bits_per_sample': entry.get('bits_per_sample', entry.get('bitsPerSample', 16.0)),
                'space_savings': entry.get('space_savings', entry.get('spaceSavings', 0.0)),
                'optimal_m': m1,            # Use m1 for numerical plotting
                'optimal_m_ch1': m1,        # Store separately for reporting
                'optimal_m_ch2': m2,        # Store separately for reporting
                'encode_time': encode_time,
                'decode_time': decode_time,
                'original_size': entry.get('original_size', entry.get('originalSize', 0)),
                'compressed_size': entry.get('compressed_size', entry.get('compressedSize', 0)),
                'reconstruction_perfect': entry.get('reconstruction_perfect', entry.get('reconstructionPerfect', True)),
                'sample_rate': entry.get('sample_rate', entry.get('sampleRate', 0)),
                'num_samples': entry.get('num_samples', entry.get('numSamples', 0)),
                'channels': entry.get('num_channels', entry.get('numChannels', 1))
            }
        
        print(f"✅ Successfully parsed JSON file: {filename}")
        print(f"   Found {len(stats)} test results")
        return stats
        
    except json.JSONDecodeError as e:
        print(f"❌ JSON parsing error: {e}")
        return None
    except Exception as e:
        print(f"❌ Error reading JSON file: {e}")
        import traceback
        traceback.print_exc()
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
    
    # Updated pattern to capture two optimal_m values if present (or just one)
    # NOTE: This text parser is designed for the OLD C++ text output which had one 'Optimal m' line.
    # It will continue to use one 'optimal_m' for text output.
    pattern = r'(?:Audio Test|Test): (\w+).*?Compression ratio:\s+([\d.]+):1.*?Bits per (?:sample|pixel):\s+([\d.]+).*?Space savings:\s+([\d.]+)%.*?Optimal m \(Ch1\):\s+(\d+).*?(?:Optimal m \(Ch2\):\s+(\d+))?.*?Encoding time:\s+(\d+) ms.*?Decoding time:\s+(\d+) ms'
    
    # Reverting to simpler pattern since the C++ text output likely only shows one m
    # as originally designed. The JSON is the reliable source for two channels.
    pattern = r'(?:Audio Test|Test): (.*?).*?Compression ratio:\s+([\d.]+):1.*?Bits per (?:sample|pixel):\s+([\d.]+).*?Space savings:\s+([\d.]+)%.*?Optimal m \((?:Ch1|Ch2)\):\s+(\d+).*?Encoding time:\s+(\d+) ms.*?Decoding time:\s+(\d+) ms'
    
    matches = re.findall(pattern, content, re.DOTALL)
    
    for match in matches:
        # predictor is the full name with stereo/predictor
        predictor = match[0].strip() 
        stats[predictor] = {
            'compression_ratio': float(match[1]),
            'bits_per_sample': float(match[2]),
            'space_savings': float(match[3]),
            'optimal_m': int(match[4]), # Only capturing the first M found
            'optimal_m_ch1': int(match[4]), # Adding ch1/ch2 for consistency
            'optimal_m_ch2': int(match[4]),
            'encode_time': int(match[5]),
            'decode_time': int(match[6])
        }
    
    if stats:
        print(f"✅ Successfully parsed text output: {filename}")
        print(f"   Found {len(stats)} test results")
    
    return stats

def calculate_velocities(stats):
    """Calculate compression and decompression velocities (MB/s)"""
    # Calculate audio size from original_size if available
    test_audio_mb = 0.1  # Default: ~100KB typical test audio
    
    # Try to get actual size from first entry
    for predictor in stats:
        if 'original_size' in stats[predictor] and stats[predictor]['original_size'] > 0:
            test_audio_mb = stats[predictor]['original_size'] / (1024 * 1024)
            break
    
    for predictor in stats:
        encode_time_s = stats[predictor]['encode_time'] / 1000.0
        decode_time_s = stats[predictor]['decode_time'] / 1000.0
        
        # Velocity = size / time (MB/s)
        stats[predictor]['encode_velocity'] = test_audio_mb / encode_time_s if encode_time_s > 0 else 0
        stats[predictor]['decode_velocity'] = test_audio_mb / decode_time_s if decode_time_s > 0 else 0
    
    return stats

def create_enhanced_visualizations(stats, output_filename='codec_performance_audio_enhanced.png'):
    """Create comprehensive performance graphs for audio codecs"""
    
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
    
    fig.suptitle('Audio Codec Performance Analysis - Golomb Coding', 
                 fontsize=18, fontweight='bold', y=0.98)
    
    # Color palette - audio-themed colors
    colors = {
        'primary': '#1e88e5',      # Blue
        'secondary': '#43a047',    # Green
        'tertiary': '#e53935',     # Red
        'quaternary': '#8e24aa',   # Purple
        'quinary': '#fb8c00',      # Orange
        'senary': '#00acc1'        # Cyan
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
    
    # 2. Bits per Sample (Top Right)
    ax2 = fig.add_subplot(gs[0, 1])
    bits_per_sample = [stats[p]['bits_per_sample'] for p in predictors]
    bars2 = ax2.bar(predictors, bits_per_sample, color=colors['secondary'], 
                    alpha=0.8, edgecolor='black', linewidth=1.5)
    ax2.set_title('Bits per Sample', fontweight='bold', fontsize=12, pad=10)
    ax2.set_ylabel('Bits (lower is better)', fontweight='bold')
    ax2.set_xlabel('Predictor Type', fontweight='bold')
    ax2.grid(True, alpha=0.3, axis='y')
    ax2.axhline(y=16.0, color='r', linestyle='--', alpha=0.5, linewidth=2, label='Original (16 bps)')
    ax2.legend(loc='upper right')
    
    # Add value labels on bars
    for bar, val in zip(bars2, bits_per_sample):
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
    optimal_m = [stats[p]['optimal_m'] for p in predictors] # Uses m1
    bars4 = ax4.bar(predictors, optimal_m, color=colors['quaternary'], 
                    alpha=0.8, edgecolor='black', linewidth=1.5)
    ax4.set_title('Optimal m Parameter (Golomb)', fontweight='bold', fontsize=12, pad=10)
    ax4.set_ylabel('m value (Ch 1)', fontweight='bold') # Updated label
    ax4.set_xlabel('Predictor Type', fontweight='bold')
    ax4.grid(True, alpha=0.3, axis='y')
    
    # Add value labels and color coding based on m value
    for i, (bar, val) in enumerate(zip(bars4, optimal_m)):
        predictor_name = predictors[i]
        m1 = stats[predictor_name]['optimal_m_ch1']
        m2 = stats[predictor_name]['optimal_m_ch2']
        
        # Display m1/m2 if they are different
        label_text = str(m1) if m1 == m2 else f"{m1}/{m2}"
        
        height = bar.get_height()
        ax4.text(bar.get_x() + bar.get_width()/2., height,
                label_text, ha='center', va='bottom', fontweight='bold', fontsize=10)
        
        # Color code based on m value range (using m1)
        if val < 10:
            bar.set_color('#e53935')  # Red for low m (high compression)
        elif val < 20:
            bar.set_color('#fb8c00')  # Orange for medium m
        else:
            bar.set_color('#8e24aa')  # Purple for high m (lower compression)
    
    ax4.tick_params(axis='x', rotation=45)
    
    # Add legend for m value ranges
    from matplotlib.patches import Patch
    legend_elements = [
        Patch(facecolor='#e53935', label='Low m (<10): High compression'),
        Patch(facecolor='#fb8c00', label='Medium m (10-20): Balanced'),
        Patch(facecolor='#8e24aa', label='High m (>20): Lower compression')
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
    
    # Calculate velocities once (already done in create_enhanced_visualizations, but good to be safe)
    if 'encode_velocity' not in list(stats.values())[0]:
        stats = calculate_velocities(stats)
        
    # Find best performers
    best_ratio = max(stats.items(), key=lambda x: x[1]['compression_ratio'])
    best_speed = max(stats.items(), key=lambda x: x[1]['encode_velocity'])
    # Finding lowest m based on Channel 1
    lowest_m = min(stats.items(), key=lambda x: x[1]['optimal_m_ch1']) 
    best_bps = min(stats.items(), key=lambda x: x[1]['bits_per_sample'])
    
    print(f"\n🏆 Best Compression Ratio: {best_ratio[0]}")
    print(f"   └─ {best_ratio[1]['compression_ratio']:.3f}:1 ({best_ratio[1]['space_savings']:.1f}% savings)")
    print(f"   └─ {best_ratio[1]['bits_per_sample']:.3f} bits/sample")
    
    print(f"\n⚡ Fastest Compression: {best_speed[0]}")
    print(f"   └─ {best_speed[1]['encode_velocity']:.3f} MB/s encoding")
    print(f"   └─ {best_speed[1]['decode_velocity']:.3f} MB/s decoding")
    
    print(f"\n🎯 Most Efficient Predictor (lowest m1): {lowest_m[0]}")
    print(f"   └─ m1/m2 = {lowest_m[1]['optimal_m_ch1']}/{lowest_m[1]['optimal_m_ch2']} (lower m = better prediction)")
    
    print(f"\n📉 Lowest Bits/Sample: {best_bps[0]}")
    print(f"   └─ {best_bps[1]['bits_per_sample']:.3f} bits/sample (vs 16.0 original)")
    
    print("\n" + "-"*80)
    print(" " * 30 + "DETAILED METRICS")
    print("-"*80)
    # Updated header to show m1/m2
    print(f"{'Predictor':<12} {'Ratio':<12} {'BPS':<10} {'m1/m2':<8} {'Enc(MB/s)':<12} {'Dec(MB/s)':<12}")
    print("-"*80)
    
    # Sort by compression ratio (best first)
    sorted_predictors = sorted(stats.items(), key=lambda x: x[1]['compression_ratio'], reverse=True)
    
    for predictor, s in sorted_predictors:
        perfect = "✓" if s.get('reconstruction_perfect', True) else "✗"
        # Combine m1 and m2 for display
        m_val = f"{s['optimal_m_ch1']}/{s['optimal_m_ch2']}"
        
        print(f"{predictor:<12} {s['compression_ratio']:>6.3f}:1     {s['bits_per_sample']:>6.3f}    "
              f"{m_val:>6}     {s['encode_velocity']:>8.3f}      {s['decode_velocity']:>8.3f}  {perfect}")
    
    print("="*80)
    
    # Additional statistics
    avg_ratio = sum(s['compression_ratio'] for s in stats.values()) / len(stats)
    avg_bps = sum(s['bits_per_sample'] for s in stats.values()) / len(stats)
    avg_savings = sum(s['space_savings'] for s in stats.values()) / len(stats)
    
    print(f"\n📊 Average Performance:")
    print(f"   ├─ Compression Ratio: {avg_ratio:.3f}:1")
    print(f"   ├─ Bits per Sample: {avg_bps:.3f}")
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
    print(" " * 20 + "Audio Codec Performance Visualizer")
    print(" " * 25 + "Golomb Coding Analysis")
    print("=" * 80)
    
    stats = None
    
    if len(sys.argv) > 1:
        filename = sys.argv[1]
        
        if not os.path.exists(filename):
            print(f"\n❌ Error: File not found: {filename}")
            print("\nUsage: python visualizerAudio.py <input_file>")
            print("  Supported formats:")
            print("    - JSON results file (audio_codec_results.json)")
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
        default_files = ['audio_codec_results.json', 'test_output.txt', 'audio_test_output.txt']
        
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
            print("\nUsage: python visualizerAudio.py <input_file>")
            print("\nSupported input files:")
            print("  • audio_codec_results.json (JSON format)")
            print("  • test_output.txt (captured program output)")
            print("  • audio_test_output.txt (captured audio test output)")
            print("\nTo capture test output:")
            print("  make run-audio FILE=./audio/sample.wav > audio_test_output.txt")
            print("  python visualizerAudio.py audio_test_output.txt")
            print("\nOr if JSON results exist:")
            print("  python visualizerAudio.py audio_codec_results.json")
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
    print("   • codec_performance_audio_enhanced.png - Comprehensive 6-panel visualization")

if __name__ == "__main__":
    main()
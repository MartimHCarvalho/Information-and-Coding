#include "../includes/preprocessor.hpp"
#include <cmath>
#include <map>
#include <cstring>

std::vector<uint8_t> Preprocessor::preprocess(const std::vector<uint8_t>& data, Strategy strategy) {
    switch (strategy) {
        case Strategy::NONE: return data;
        case Strategy::BYTE_REORDER: return byteReorder(data);
        case Strategy::DELTA_ENCODING: return deltaEncode(data);
        case Strategy::BF16_TO_FP16: return bf16ToFp16(data);
        case Strategy::COMBINED: return combinedPreprocess(data);
        case Strategy::BYTE_REORDER_DELTA: return byteReorderDelta(data);
        case Strategy::BIT_PLANE_SEPARATION: return bitPlaneSeparation(data);
    }
    return data;
}

std::vector<uint8_t> Preprocessor::deprocess(const std::vector<uint8_t>& data, Strategy strategy) {
    switch (strategy) {
        case Strategy::NONE: return data;
        case Strategy::BYTE_REORDER: return byteDeorder(data);
        case Strategy::DELTA_ENCODING: return deltaDecode(data);
        case Strategy::BF16_TO_FP16: return fp16ToBf16(data);
        case Strategy::COMBINED: return combinedDeprocess(data);
        case Strategy::BYTE_REORDER_DELTA: return byteReorderDeltaInverse(data);
        case Strategy::BIT_PLANE_SEPARATION: return bitPlaneReconstruction(data);
    }
    return data;
}

std::vector<uint8_t> Preprocessor::byteReorder(const std::vector<uint8_t>& data) {
    size_t num_values = data.size() / 2;
    std::vector<uint8_t> reordered(data.size());

    // Two-pass: sequential writes for cache efficiency
    size_t src_idx = 0;
    // First pass: copy all high bytes
    for (size_t i = 0; i < num_values; ++i) {
        reordered[i] = data[src_idx];
        src_idx += 2;
    }
    // Second pass: copy all low bytes
    src_idx = 1;
    for (size_t i = 0; i < num_values; ++i) {
        reordered[num_values + i] = data[src_idx];
        src_idx += 2;
    }
    return reordered;
}

std::vector<uint8_t> Preprocessor::byteDeorder(const std::vector<uint8_t>& data) {
    size_t num_values = data.size() / 2;
    std::vector<uint8_t> original(data.size());

    // Two-pass interleaving
    size_t dst_idx = 0;
    // First pass: place high bytes
    for (size_t i = 0; i < num_values; ++i) {
        original[dst_idx] = data[i];
        dst_idx += 2;
    }
    // Second pass: place low bytes
    dst_idx = 1;
    for (size_t i = 0; i < num_values; ++i) {
        original[dst_idx] = data[num_values + i];
        dst_idx += 2;
    }
    return original;
}

std::vector<uint8_t> Preprocessor::deltaEncode(const std::vector<uint8_t>& data) {
    // Not used in current implementation - kept for future experimentation
    if (data.size() < 4) return data;  // Need at least 2 int16 values

    std::vector<uint8_t> encoded;
    encoded.reserve(data.size());

    // Copy first value unchanged
    encoded.push_back(data[0]);
    encoded.push_back(data[1]);

    size_t num_values = data.size() / 2;
    for (size_t i = 1; i < num_values; ++i) {
        int16_t prev, curr;
        memcpy(&prev, &data[(i - 1) * 2], 2);
        memcpy(&curr, &data[i * 2], 2);
        int16_t delta = curr - prev;

        uint8_t delta_bytes[2];
        memcpy(delta_bytes, &delta, 2);
        encoded.push_back(delta_bytes[0]);
        encoded.push_back(delta_bytes[1]);
    }
    return encoded;
}

std::vector<uint8_t> Preprocessor::deltaDecode(const std::vector<uint8_t>& data) {
    // Not used in current implementation - kept for future experimentation
    if (data.size() < 4) return data;  // Need at least 2 int16 values

    std::vector<uint8_t> decoded;
    decoded.reserve(data.size());

    // First value is unchanged
    decoded.push_back(data[0]);
    decoded.push_back(data[1]);

    size_t num_values = data.size() / 2;
    for (size_t i = 1; i < num_values; ++i) {
        int16_t prev, delta;
        memcpy(&prev, &decoded[(i - 1) * 2], 2);
        memcpy(&delta, &data[i * 2], 2);
        int16_t curr = prev + delta;

        uint8_t curr_bytes[2];
        memcpy(curr_bytes, &curr, 2);
        decoded.push_back(curr_bytes[0]);
        decoded.push_back(curr_bytes[1]);
    }
    return decoded;
}

std::vector<uint8_t> Preprocessor::bf16ToFp16(const std::vector<uint8_t>& data) {
    size_t num_values = data.size() / 2;
    std::vector<uint8_t> fp16_data(data.size());

    for (size_t i = 0; i < num_values; ++i) {
        uint16_t bf16;
        memcpy(&bf16, &data[i * 2], 2);

        uint32_t fp32 = static_cast<uint32_t>(bf16) << 16;
        uint32_t sign = (fp32 >> 31) & 0x1;
        int32_t exp = ((fp32 >> 23) & 0xFF) - 127;
        uint32_t mantissa = fp32 & 0x7FFFFF;

        uint16_t fp16;
        if (exp < -14) {
            fp16 = sign << 15;
        } else if (exp > 15) {
            fp16 = (sign << 15) | 0x7C00;
        } else {
            uint16_t fp16_exp = (exp + 15) & 0x1F;
            uint16_t fp16_mantissa = mantissa >> 13;
            fp16 = (sign << 15) | (fp16_exp << 10) | fp16_mantissa;
        }

        memcpy(&fp16_data[i * 2], &fp16, 2);
    }
    return fp16_data;
}

std::vector<uint8_t> Preprocessor::fp16ToBf16(const std::vector<uint8_t>& data) {
    size_t num_values = data.size() / 2;
    std::vector<uint8_t> bf16_data(data.size());

    for (size_t i = 0; i < num_values; ++i) {
        uint16_t fp16;
        memcpy(&fp16, &data[i * 2], 2);

        uint32_t sign = (fp16 >> 15) & 0x1;
        uint32_t exp = (fp16 >> 10) & 0x1F;
        uint32_t mantissa = fp16 & 0x3FF;

        uint32_t fp32;
        if (exp == 0) {
            fp32 = sign << 31;
        } else if (exp == 0x1F) {
            fp32 = (sign << 31) | 0x7F800000 | (mantissa << 13);
        } else {
            uint32_t fp32_exp = exp - 15 + 127;
            fp32 = (sign << 31) | (fp32_exp << 23) | (mantissa << 13);
        }

        uint16_t bf16 = fp32 >> 16;
        memcpy(&bf16_data[i * 2], &bf16, 2);
    }
    return bf16_data;
}

std::vector<uint8_t> Preprocessor::combinedPreprocess(const std::vector<uint8_t>& data) {
    auto temp = bf16ToFp16(data);
    return deltaEncode(temp);
}

std::vector<uint8_t> Preprocessor::combinedDeprocess(const std::vector<uint8_t>& data) {
    auto temp = deltaDecode(data);
    return fp16ToBf16(temp);
}

double Preprocessor::calculateEntropy(const std::vector<uint8_t>& data) {
    if (data.empty()) return 0.0;

    // Sample-based entropy calculation for large datasets
    const size_t MAX_SAMPLES = 10000000; // 10MB sample
    const size_t sample_size = std::min(data.size(), MAX_SAMPLES);
    const size_t stride = data.size() / sample_size;

    std::map<uint8_t, size_t> freq;
    if (stride <= 1) {
        // Small dataset, use all data
        for (size_t i = 0; i < sample_size; ++i) freq[data[i]]++;
    } else {
        // Large dataset, use uniform sampling
        for (size_t i = 0; i < data.size(); i += stride) {
            freq[data[i]]++;
        }
    }

    double entropy = 0.0;
    double size = static_cast<double>(freq.size() > 0 ? sample_size : data.size());
    for (const auto& pair : freq) {
        double prob = static_cast<double>(pair.second) / size;
        entropy -= prob * std::log2(prob);
    }
    return entropy;
}

std::string Preprocessor::getStrategyName(Strategy strategy) {
    switch (strategy) {
        case Strategy::NONE: return "None";
        case Strategy::BYTE_REORDER: return "ByteReorder";
        case Strategy::DELTA_ENCODING: return "DeltaEncoding";
        case Strategy::BF16_TO_FP16: return "BF16toFP16";
        case Strategy::COMBINED: return "Combined";
        case Strategy::BYTE_REORDER_DELTA: return "ByteReorderDelta";
        case Strategy::BIT_PLANE_SEPARATION: return "BitPlaneSeparation";
    }
    return "Unknown";
}

// ============================================================================
// NEW ADVANCED PREPROCESSING STRATEGIES
// ============================================================================

/**
 * BYTE_REORDER_DELTA: Combines byte reordering with delta encoding
 *
 * Strategy:
 * 1. Apply byte reordering (separates high/low bytes of BF16 values)
 * 2. Apply delta encoding to high bytes (they cluster around 0x3c, 0xbc)
 * 3. Apply delta encoding to low bytes separately
 *
 * Rationale: After byte reordering, high bytes are very similar, so deltas
 * will be small (often zero), leading to better compression.
 */
std::vector<uint8_t> Preprocessor::byteReorderDelta(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return data;  // Need at least 2 BF16 values

    // Step 1: Apply byte reordering first
    std::vector<uint8_t> reordered = byteReorder(data);

    // Step 2: Apply delta encoding to each half independently
    size_t half_size = reordered.size() / 2;
    std::vector<uint8_t> result;
    result.reserve(reordered.size());

    // Delta encode high bytes (first half)
    result.push_back(reordered[0]);  // First high byte unchanged
    for (size_t i = 1; i < half_size; ++i) {
        int16_t delta = static_cast<int16_t>(reordered[i]) - static_cast<int16_t>(reordered[i - 1]);
        result.push_back(static_cast<uint8_t>(delta));
    }

    // Delta encode low bytes (second half)
    result.push_back(reordered[half_size]);  // First low byte unchanged
    for (size_t i = half_size + 1; i < reordered.size(); ++i) {
        int16_t delta = static_cast<int16_t>(reordered[i]) - static_cast<int16_t>(reordered[i - 1]);
        result.push_back(static_cast<uint8_t>(delta));
    }

    return result;
}

/**
 * BYTE_REORDER_DELTA Inverse: Reverses the byte reorder + delta encoding
 */
std::vector<uint8_t> Preprocessor::byteReorderDeltaInverse(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return data;

    size_t half_size = data.size() / 2;
    std::vector<uint8_t> decoded;
    decoded.reserve(data.size());

    // Decode high bytes (first half)
    decoded.push_back(data[0]);  // First high byte is unchanged
    for (size_t i = 1; i < half_size; ++i) {
        int16_t prev = static_cast<int16_t>(decoded[i - 1]);
        int16_t delta = static_cast<int8_t>(data[i]);  // Interpret as signed
        decoded.push_back(static_cast<uint8_t>(prev + delta));
    }

    // Decode low bytes (second half)
    decoded.push_back(data[half_size]);  // First low byte is unchanged
    for (size_t i = half_size + 1; i < data.size(); ++i) {
        int16_t prev = static_cast<int16_t>(decoded[i - 1]);
        int16_t delta = static_cast<int8_t>(data[i]);  // Interpret as signed
        decoded.push_back(static_cast<uint8_t>(prev + delta));
    }

    // Step 2: Reverse byte reordering
    return byteDeorder(decoded);
}

/**
 * BIT_PLANE_SEPARATION: Separates data by bit significance
 *
 * Strategy:
 * Instead of separating bytes, separate individual bits across all values.
 * For BF16 format [sign:1][exponent:8][mantissa:7]:
 * - All sign bits grouped together (highly compressible)
 * - All exponent bits grouped by position (cluster around specific values)
 * - All mantissa bits grouped by position
 *
 * Result: 16 bit planes, each highly compressible
 *
 * Note: Output size may be larger than input due to byte alignment of bit planes
 */
// Suppress false positive warnings from GCC 13 on small vector copies
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
std::vector<uint8_t> Preprocessor::bitPlaneSeparation(const std::vector<uint8_t>& data) {
    if (data.size() < 2) return data;

    size_t num_values = data.size() / 2;  // Number of BF16 values
    size_t bytes_per_plane = (num_values + 7) / 8;  // Round up to whole bytes
    std::vector<uint8_t> separated(16 * bytes_per_plane, 0);  // 16 bit planes

    // For each BF16 value
    for (size_t val_idx = 0; val_idx < num_values; ++val_idx) {
        // Read the 16-bit value in little-endian
        uint16_t value = static_cast<uint16_t>(data[val_idx * 2]) |
                        (static_cast<uint16_t>(data[val_idx * 2 + 1]) << 8);

        // Extract each of the 16 bits and place into corresponding plane
        for (int bit_pos = 0; bit_pos < 16; ++bit_pos) {
            uint8_t bit = (value >> bit_pos) & 1;

            if (bit) {
                size_t plane_offset = bit_pos * bytes_per_plane;
                size_t byte_in_plane = val_idx / 8;
                size_t bit_in_byte = val_idx % 8;

                separated[plane_offset + byte_in_plane] |= (1 << bit_in_byte);
            }
        }
    }

    return separated;  // Keep full size, don't trim
}
#pragma GCC diagnostic pop

/**
 * BIT_PLANE_SEPARATION Inverse: Reconstructs data from bit planes
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
std::vector<uint8_t> Preprocessor::bitPlaneReconstruction(const std::vector<uint8_t>& data) {
    if (data.size() < 2) return data;

    // Calculate number of values from bit plane size
    size_t bytes_per_plane = data.size() / 16;
    size_t num_values = bytes_per_plane * 8;  // Maximum possible values

    // Actual output size
    size_t output_size = num_values * 2;
    std::vector<uint8_t> reconstructed(output_size, 0);

    // For each BF16 value
    for (size_t val_idx = 0; val_idx < num_values; ++val_idx) {
        uint16_t value = 0;

        // Extract each bit from its plane
        for (int bit_pos = 0; bit_pos < 16; ++bit_pos) {
            size_t plane_offset = bit_pos * bytes_per_plane;
            size_t byte_in_plane = val_idx / 8;
            size_t bit_in_byte = val_idx % 8;

            if (plane_offset + byte_in_plane < data.size()) {
                uint8_t bit = (data[plane_offset + byte_in_plane] >> bit_in_byte) & 1;

                if (bit) {
                    value |= (1 << bit_pos);
                }
            }
        }

        // Write the 16-bit value in little-endian
        if (val_idx * 2 + 1 < reconstructed.size()) {
            reconstructed[val_idx * 2] = value & 0xFF;
            reconstructed[val_idx * 2 + 1] = (value >> 8) & 0xFF;
        }
    }

    return reconstructed;
}
#pragma GCC diagnostic pop

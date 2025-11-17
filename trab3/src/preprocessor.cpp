#include "preprocessor.hpp"
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
    }
    return data;
}

std::vector<uint8_t> Preprocessor::byteReorder(const std::vector<uint8_t>& data) {
    size_t num_values = data.size() / 2;
    std::vector<uint8_t> reordered(data.size());
    for (size_t i = 0; i < num_values; ++i) {
        reordered[i] = data[i * 2];
        reordered[num_values + i] = data[i * 2 + 1];
    }
    return reordered;
}

std::vector<uint8_t> Preprocessor::byteDeorder(const std::vector<uint8_t>& data) {
    size_t num_values = data.size() / 2;
    std::vector<uint8_t> original(data.size());
    for (size_t i = 0; i < num_values; ++i) {
        original[i * 2] = data[i];
        original[i * 2 + 1] = data[num_values + i];
    }
    return original;
}

std::vector<uint8_t> Preprocessor::deltaEncode(const std::vector<uint8_t>& data) {
    if (data.size() < 2) return data;
    std::vector<uint8_t> encoded(data.size());
    encoded[0] = data[0];
    encoded[1] = data[1];

    size_t num_values = data.size() / 2;
    for (size_t i = 1; i < num_values; ++i) {
        int16_t prev, curr;
        memcpy(&prev, &data[(i - 1) * 2], 2);
        memcpy(&curr, &data[i * 2], 2);
        int16_t delta = curr - prev;
        memcpy(&encoded[i * 2], &delta, 2);
    }
    return encoded;
}

std::vector<uint8_t> Preprocessor::deltaDecode(const std::vector<uint8_t>& data) {
    if (data.size() < 2) return data;
    std::vector<uint8_t> decoded(data.size());
    decoded[0] = data[0];
    decoded[1] = data[1];

    size_t num_values = data.size() / 2;
    for (size_t i = 1; i < num_values; ++i) {
        int16_t prev, delta;
        memcpy(&prev, &decoded[(i - 1) * 2], 2);
        memcpy(&delta, &data[i * 2], 2);
        int16_t curr = prev + delta;
        memcpy(&decoded[i * 2], &curr, 2);
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

    std::map<uint8_t, size_t> freq;
    for (uint8_t byte : data) freq[byte]++;

    double entropy = 0.0;
    double size = static_cast<double>(data.size());
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
    }
    return "Unknown";
}

#ifndef PREPROCESSOR_HPP
#define PREPROCESSOR_HPP

#include <vector>
#include <cstdint>
#include <string>

class Preprocessor {
public:
    enum class Strategy {
        NONE,
        BYTE_REORDER,
        DELTA_ENCODING,
        BF16_TO_FP16,
        COMBINED
    };

    Preprocessor() = default;
    ~Preprocessor() = default;

    std::vector<uint8_t> preprocess(const std::vector<uint8_t>& data, Strategy strategy);
    std::vector<uint8_t> deprocess(const std::vector<uint8_t>& data, Strategy strategy);

    static double calculateEntropy(const std::vector<uint8_t>& data);
    static std::string getStrategyName(Strategy strategy);

private:
    std::vector<uint8_t> byteReorder(const std::vector<uint8_t>& data);
    std::vector<uint8_t> byteDeorder(const std::vector<uint8_t>& data);
    std::vector<uint8_t> deltaEncode(const std::vector<uint8_t>& data);
    std::vector<uint8_t> deltaDecode(const std::vector<uint8_t>& data);
    std::vector<uint8_t> bf16ToFp16(const std::vector<uint8_t>& data);
    std::vector<uint8_t> fp16ToBf16(const std::vector<uint8_t>& data);
    std::vector<uint8_t> combinedPreprocess(const std::vector<uint8_t>& data);
    std::vector<uint8_t> combinedDeprocess(const std::vector<uint8_t>& data);
};

#endif

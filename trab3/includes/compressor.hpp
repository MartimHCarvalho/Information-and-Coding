#ifndef COMPRESSOR_HPP
#define COMPRESSOR_HPP

#include <vector>
#include <cstdint>
#include <string>
#include "preprocessor.hpp"

class Compressor {
public:
    enum class OperationPoint {
        FAST,
        BALANCED,
        MAXIMUM
    };

    Compressor() = default;
    ~Compressor() = default;

    std::vector<uint8_t> compress(const std::vector<uint8_t>& data, OperationPoint op_point);
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed_data, OperationPoint op_point);

    bool writeCompressedFile(const std::string& filepath, const std::string& header,
                            const std::vector<uint8_t>& compressed_data, OperationPoint op_point);
    bool readCompressedFile(const std::string& filepath, std::string& header,
                           std::vector<uint8_t>& compressed_data, OperationPoint& op_point);

    static std::string getOperationPointName(OperationPoint op_point);

private:
    Preprocessor preprocessor_;

    std::vector<uint8_t> compressZSTD(const std::vector<uint8_t>& data, int level);
    std::vector<uint8_t> decompressZSTD(const std::vector<uint8_t>& data);
    std::vector<uint8_t> compressLZMA(const std::vector<uint8_t>& data, int level);
    std::vector<uint8_t> decompressLZMA(const std::vector<uint8_t>& data);

    Preprocessor::Strategy getPreprocessingStrategy(OperationPoint op_point);
};

#endif

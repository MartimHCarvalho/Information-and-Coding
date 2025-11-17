#include "compressor.hpp"
#include <zstd.h>
#include <lzma.h>
#include <fstream>
#include <iostream>
#include <stdexcept>

std::vector<uint8_t> Compressor::compress(const std::vector<uint8_t>& data, OperationPoint op_point) {
    Preprocessor::Strategy strategy = getPreprocessingStrategy(op_point);
    std::vector<uint8_t> preprocessed = preprocessor_.preprocess(data, strategy);

    switch (op_point) {
        case OperationPoint::FAST: return compressZSTD(preprocessed, 10);
        case OperationPoint::BALANCED: return compressZSTD(preprocessed, 15);
        case OperationPoint::MAXIMUM: return compressLZMA(preprocessed, 9);
    }
    return preprocessed;
}

std::vector<uint8_t> Compressor::decompress(const std::vector<uint8_t>& compressed_data, OperationPoint op_point) {
    std::vector<uint8_t> decompressed;

    switch (op_point) {
        case OperationPoint::FAST:
        case OperationPoint::BALANCED:
            decompressed = decompressZSTD(compressed_data);
            break;
        case OperationPoint::MAXIMUM:
            decompressed = decompressLZMA(compressed_data);
            break;
    }

    Preprocessor::Strategy strategy = getPreprocessingStrategy(op_point);
    return preprocessor_.deprocess(decompressed, strategy);
}

std::vector<uint8_t> Compressor::compressZSTD(const std::vector<uint8_t>& data, int level) {
    size_t bound = ZSTD_compressBound(data.size());
    std::vector<uint8_t> compressed(bound);

    size_t size = ZSTD_compress(compressed.data(), bound, data.data(), data.size(), level);
    if (ZSTD_isError(size)) {
        throw std::runtime_error("ZSTD compression failed");
    }

    compressed.resize(size);
    return compressed;
}

std::vector<uint8_t> Compressor::decompressZSTD(const std::vector<uint8_t>& data) {
    unsigned long long size = ZSTD_getFrameContentSize(data.data(), data.size());
    if (size == ZSTD_CONTENTSIZE_ERROR || size == ZSTD_CONTENTSIZE_UNKNOWN) {
        throw std::runtime_error("ZSTD decompression error");
    }

    std::vector<uint8_t> decompressed(size);
    size_t result = ZSTD_decompress(decompressed.data(), size, data.data(), data.size());
    if (ZSTD_isError(result)) {
        throw std::runtime_error("ZSTD decompression failed");
    }

    return decompressed;
}

std::vector<uint8_t> Compressor::compressLZMA(const std::vector<uint8_t>& data, int level) {
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_options_lzma options;
    lzma_lzma_preset(&options, level);

    lzma_filter filters[] = {
        { .id = LZMA_FILTER_LZMA2, .options = &options },
        { .id = LZMA_VLI_UNKNOWN, .options = nullptr }
    };

    if (lzma_stream_encoder(&strm, filters, LZMA_CHECK_CRC64) != LZMA_OK) {
        throw std::runtime_error("LZMA encoder init failed");
    }

    std::vector<uint8_t> compressed(data.size() + 1024);
    strm.next_in = data.data();
    strm.avail_in = data.size();
    strm.next_out = compressed.data();
    strm.avail_out = compressed.size();

    if (lzma_code(&strm, LZMA_FINISH) != LZMA_STREAM_END) {
        lzma_end(&strm);
        throw std::runtime_error("LZMA compression failed");
    }

    size_t compressed_size = strm.total_out;
    lzma_end(&strm);
    compressed.resize(compressed_size);
    return compressed;
}

std::vector<uint8_t> Compressor::decompressLZMA(const std::vector<uint8_t>& data) {
    lzma_stream strm = LZMA_STREAM_INIT;

    if (lzma_stream_decoder(&strm, UINT64_MAX, LZMA_CONCATENATED) != LZMA_OK) {
        throw std::runtime_error("LZMA decoder init failed");
    }

    std::vector<uint8_t> decompressed(data.size() * 2);
    strm.next_in = data.data();
    strm.avail_in = data.size();
    strm.next_out = decompressed.data();
    strm.avail_out = decompressed.size();

    lzma_ret ret;
    while (true) {
        ret = lzma_code(&strm, LZMA_FINISH);
        if (ret == LZMA_STREAM_END) break;
        if (ret == LZMA_BUF_ERROR) {
            size_t current = decompressed.size();
            decompressed.resize(current * 2);
            strm.next_out = decompressed.data() + current;
            strm.avail_out = current;
        } else if (ret != LZMA_OK) {
            lzma_end(&strm);
            throw std::runtime_error("LZMA decompression failed");
        }
    }

    decompressed.resize(strm.total_out);
    lzma_end(&strm);
    return decompressed;
}

Preprocessor::Strategy Compressor::getPreprocessingStrategy(OperationPoint op_point) {
    switch (op_point) {
        case OperationPoint::FAST: return Preprocessor::Strategy::BYTE_REORDER;
        case OperationPoint::BALANCED: return Preprocessor::Strategy::DELTA_ENCODING;
        case OperationPoint::MAXIMUM: return Preprocessor::Strategy::COMBINED;
    }
    return Preprocessor::Strategy::NONE;
}

std::string Compressor::getOperationPointName(OperationPoint op_point) {
    switch (op_point) {
        case OperationPoint::FAST: return "Fast";
        case OperationPoint::BALANCED: return "Balanced";
        case OperationPoint::MAXIMUM: return "Maximum";
    }
    return "Unknown";
}

bool Compressor::writeCompressedFile(const std::string& filepath, const std::string& header,
                                     const std::vector<uint8_t>& compressed_data, OperationPoint op_point) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;

    file.write("STCMP", 5);
    uint8_t version = 1;
    file.write(reinterpret_cast<const char*>(&version), 1);
    uint8_t op = static_cast<uint8_t>(op_point);
    file.write(reinterpret_cast<const char*>(&op), 1);

    uint64_t header_size = header.size();
    file.write(reinterpret_cast<const char*>(&header_size), 8);
    file.write(header.data(), header.size());

    uint64_t compressed_size = compressed_data.size();
    file.write(reinterpret_cast<const char*>(&compressed_size), 8);
    file.write(reinterpret_cast<const char*>(compressed_data.data()), compressed_data.size());

    file.close();
    return true;
}

bool Compressor::readCompressedFile(const std::string& filepath, std::string& header,
                                    std::vector<uint8_t>& compressed_data, OperationPoint& op_point) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;

    char magic[6] = {0};
    file.read(magic, 5);
    if (std::string(magic) != "STCMP") return false;

    uint8_t version;
    file.read(reinterpret_cast<char*>(&version), 1);
    if (version != 1) return false;

    uint8_t op;
    file.read(reinterpret_cast<char*>(&op), 1);
    op_point = static_cast<OperationPoint>(op);

    uint64_t header_size;
    file.read(reinterpret_cast<char*>(&header_size), 8);
    std::vector<char> header_buf(header_size);
    file.read(header_buf.data(), header_size);
    header = std::string(header_buf.begin(), header_buf.end());

    uint64_t compressed_size;
    file.read(reinterpret_cast<char*>(&compressed_size), 8);
    compressed_data.resize(compressed_size);
    file.read(reinterpret_cast<char*>(compressed_data.data()), compressed_size);

    file.close();
    return true;
}

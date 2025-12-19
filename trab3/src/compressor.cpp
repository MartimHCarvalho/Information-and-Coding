#include "../includes/compressor.hpp"
#include <zstd.h>
#include <lz4.h>
#include <lz4hc.h>
#include <zlib.h>
#include <lzma.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <thread>

std::vector<uint8_t> Compressor::compress(const std::vector<uint8_t>& data, 
                                           Algorithm algo,
                                           OperationPoint op_point) {
    Preprocessor::Strategy strategy = getPreprocessingStrategy(op_point);
    std::vector<uint8_t> preprocessed = preprocessor_.preprocess(data, strategy);
    
    int level = getCompressionLevel(algo, op_point);
    
    switch (algo) {
        case Algorithm::ZSTD: return compressZSTD(preprocessed, level);
        case Algorithm::LZ4: return compressLZ4(preprocessed, level);
        case Algorithm::DEFLATE: return compressDEFLATE(preprocessed, level);
        case Algorithm::LZMA: return compressLZMA(preprocessed, level);
    }
    return preprocessed;
}

std::vector<uint8_t> Compressor::decompress(const std::vector<uint8_t>& compressed_data,
                                             Algorithm algo,
                                             OperationPoint op_point) {
    std::vector<uint8_t> decompressed;

    switch (algo) {
        case Algorithm::ZSTD:
            decompressed = decompressZSTD(compressed_data);
            break;
        case Algorithm::LZ4:
            decompressed = decompressLZ4(compressed_data);
            break;
        case Algorithm::DEFLATE:
            decompressed = decompressDEFLATE(compressed_data);
            break;
        case Algorithm::LZMA:
            decompressed = decompressLZMA(compressed_data);
            break;
    }

    Preprocessor::Strategy strategy = getPreprocessingStrategy(op_point);
    return preprocessor_.deprocess(decompressed, strategy);
}

// ZSTD Implementation with Multithreading
std::vector<uint8_t> Compressor::compressZSTD(const std::vector<uint8_t>& data, int level) {
    // Create compression context
    ZSTD_CCtx* cctx = ZSTD_createCCtx();
    if (!cctx) {
        throw std::runtime_error("ZSTD: failed to create compression context");
    }

    // Set compression level
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, level);

    // Enable multithreading - use all available CPU cores
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4; // Fallback to 4 threads
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, num_threads);

    // Allocate output buffer
    size_t bound = ZSTD_compressBound(data.size());
    std::vector<uint8_t> compressed(bound);

    // Compress with context
    size_t size = ZSTD_compress2(cctx, compressed.data(), bound, data.data(), data.size());

    // Clean up
    ZSTD_freeCCtx(cctx);

    if (ZSTD_isError(size)) {
        throw std::runtime_error("ZSTD compression failed: " + std::string(ZSTD_getErrorName(size)));
    }

    compressed.resize(size);
    return compressed;
}

std::vector<uint8_t> Compressor::decompressZSTD(const std::vector<uint8_t>& data) {
    // Get decompressed size
    unsigned long long size = ZSTD_getFrameContentSize(data.data(), data.size());
    if (size == ZSTD_CONTENTSIZE_ERROR || size == ZSTD_CONTENTSIZE_UNKNOWN) {
        throw std::runtime_error("ZSTD: cannot determine decompressed size");
    }

    // Create decompression context (more efficient for large files)
    ZSTD_DCtx* dctx = ZSTD_createDCtx();
    if (!dctx) {
        throw std::runtime_error("ZSTD: failed to create decompression context");
    }

    std::vector<uint8_t> decompressed(size);
    size_t result = ZSTD_decompressDCtx(dctx, decompressed.data(), size, data.data(), data.size());

    // Clean up
    ZSTD_freeDCtx(dctx);

    if (ZSTD_isError(result)) {
        throw std::runtime_error("ZSTD decompression failed: " + std::string(ZSTD_getErrorName(result)));
    }

    return decompressed;
}

// LZ4 Implementation
std::vector<uint8_t> Compressor::compressLZ4(const std::vector<uint8_t>& data, int level) {
    int max_size = LZ4_compressBound(data.size());
    std::vector<uint8_t> compressed(max_size + 8); // +8 for original size header
    
    // Store original size in first 8 bytes for decompression
    uint64_t orig_size = data.size();
    memcpy(compressed.data(), &orig_size, 8);
    
    int compressed_size;
    if (level == 0) {
        // Fast compression
        compressed_size = LZ4_compress_default(
            reinterpret_cast<const char*>(data.data()),
            reinterpret_cast<char*>(compressed.data() + 8),
            data.size(),
            max_size
        );
    } else {
        // High compression
        compressed_size = LZ4_compress_HC(
            reinterpret_cast<const char*>(data.data()),
            reinterpret_cast<char*>(compressed.data() + 8),
            data.size(),
            max_size,
            level
        );
    }
    
    if (compressed_size <= 0) {
        throw std::runtime_error("LZ4 compression failed");
    }
    
    compressed.resize(compressed_size + 8);
    return compressed;
}

std::vector<uint8_t> Compressor::decompressLZ4(const std::vector<uint8_t>& data) {
    if (data.size() < 8) {
        throw std::runtime_error("LZ4: invalid compressed data");
    }
    
    uint64_t orig_size;
    memcpy(&orig_size, data.data(), 8);
    
    std::vector<uint8_t> decompressed(orig_size);
    int result = LZ4_decompress_safe(
        reinterpret_cast<const char*>(data.data() + 8),
        reinterpret_cast<char*>(decompressed.data()),
        data.size() - 8,
        orig_size
    );
    
    if (result < 0) {
        throw std::runtime_error("LZ4 decompression failed");
    }
    
    return decompressed;
}

// DEFLATE (zlib) Implementation
std::vector<uint8_t> Compressor::compressDEFLATE(const std::vector<uint8_t>& data, int level) {
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    
    if (deflateInit2(&stream, level, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        throw std::runtime_error("DEFLATE: initialization failed");
    }
    
    uLong bound = deflateBound(&stream, data.size());
    std::vector<uint8_t> compressed(bound);
    
    stream.next_in = const_cast<uint8_t*>(data.data());
    stream.avail_in = data.size();
    stream.next_out = compressed.data();
    stream.avail_out = compressed.size();
    
    int ret = deflate(&stream, Z_FINISH);
    deflateEnd(&stream);
    
    if (ret != Z_STREAM_END) {
        throw std::runtime_error("DEFLATE compression failed");
    }
    
    compressed.resize(stream.total_out);
    return compressed;
}

std::vector<uint8_t> Compressor::decompressDEFLATE(const std::vector<uint8_t>& data) {
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    
    if (inflateInit2(&stream, 15 + 16) != Z_OK) {
        throw std::runtime_error("DEFLATE: decompression init failed");
    }
    
    std::vector<uint8_t> decompressed(data.size() * 4); // Initial estimate
    
    stream.next_in = const_cast<uint8_t*>(data.data());
    stream.avail_in = data.size();
    stream.next_out = decompressed.data();
    stream.avail_out = decompressed.size();
    
    int ret;
    while ((ret = inflate(&stream, Z_NO_FLUSH)) == Z_OK) {
        if (stream.avail_out == 0) {
            size_t old_size = decompressed.size();
            decompressed.resize(old_size * 2);
            stream.next_out = decompressed.data() + old_size;
            stream.avail_out = old_size;
        }
    }
    
    inflateEnd(&stream);
    
    if (ret != Z_STREAM_END) {
        throw std::runtime_error("DEFLATE decompression failed");
    }
    
    decompressed.resize(stream.total_out);
    return decompressed;
}

// LZMA Implementation
std::vector<uint8_t> Compressor::compressLZMA(const std::vector<uint8_t>& data, int level) {
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_options_lzma options;
    lzma_lzma_preset(&options, level);

    lzma_filter filters[] = {
        { .id = LZMA_FILTER_LZMA2, .options = &options },
        { .id = LZMA_VLI_UNKNOWN, .options = nullptr }
    };

    if (lzma_stream_encoder(&strm, filters, LZMA_CHECK_CRC64) != LZMA_OK) {
        throw std::runtime_error("LZMA: encoder init failed");
    }

    size_t bound = data.size() + (data.size() / 10) + 65536;
    std::vector<uint8_t> compressed(bound);
    
    strm.next_in = data.data();
    strm.avail_in = data.size();
    strm.next_out = compressed.data();
    strm.avail_out = compressed.size();

    lzma_ret ret = lzma_code(&strm, LZMA_FINISH);
    if (ret != LZMA_STREAM_END) {
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
        throw std::runtime_error("LZMA: decoder init failed");
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

// Helper methods
Preprocessor::Strategy Compressor::getPreprocessingStrategy(OperationPoint op_point) {
    // Byte reordering: Separates high/low bytes of BF16 values for better compression
    // Empirical testing showed this provides optimal results (~30% space savings)
    return Preprocessor::Strategy::BYTE_REORDER;
}

int Compressor::getCompressionLevel(Algorithm algo, OperationPoint op_point) {
    switch (algo) {
        case Algorithm::ZSTD:
            switch (op_point) {
                case OperationPoint::FAST: return 3;
                case OperationPoint::MAXIMUM: return 19;
            }
            break;

        case Algorithm::LZ4:
            switch (op_point) {
                case OperationPoint::FAST: return 0;  // Fast mode
                case OperationPoint::MAXIMUM: return 12;
            }
            break;

        case Algorithm::DEFLATE:
            switch (op_point) {
                case OperationPoint::FAST: return 3;
                case OperationPoint::MAXIMUM: return 9;
            }
            break;

        case Algorithm::LZMA:
            switch (op_point) {
                case OperationPoint::FAST: return 3;
                case OperationPoint::MAXIMUM: return 9;
            }
            break;
    }
    return 9; // Default to maximum
}

std::string Compressor::getAlgorithmName(Algorithm algo) {
    switch (algo) {
        case Algorithm::ZSTD: return "ZSTD";
        case Algorithm::LZ4: return "LZ4";
        case Algorithm::DEFLATE: return "DEFLATE";
        case Algorithm::LZMA: return "LZMA";
    }
    return "Unknown";
}

std::string Compressor::getOperationPointName(OperationPoint op_point) {
    switch (op_point) {
        case OperationPoint::FAST: return "Fast";
        case OperationPoint::MAXIMUM: return "Maximum";
    }
    return "Unknown";
}

// File I/O
bool Compressor::writeCompressedFile(const std::string& filepath, 
                                     const std::string& header,
                                     const std::vector<uint8_t>& compressed_data, 
                                     Algorithm algo,
                                     OperationPoint op_point) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;

    // Magic number
    file.write("STCMP", 5);
    
    // Version
    uint8_t version = 2;  // Updated version for multi-algorithm support
    file.write(reinterpret_cast<const char*>(&version), 1);
    
    // Algorithm
    uint8_t algo_byte = static_cast<uint8_t>(algo);
    file.write(reinterpret_cast<const char*>(&algo_byte), 1);
    
    // Operation point
    uint8_t op = static_cast<uint8_t>(op_point);
    file.write(reinterpret_cast<const char*>(&op), 1);

    // Header
    uint64_t header_size = header.size();
    file.write(reinterpret_cast<const char*>(&header_size), 8);
    file.write(header.data(), header.size());

    // Compressed data
    uint64_t compressed_size = compressed_data.size();
    file.write(reinterpret_cast<const char*>(&compressed_size), 8);
    file.write(reinterpret_cast<const char*>(compressed_data.data()), compressed_data.size());

    file.close();
    return true;
}

bool Compressor::readCompressedFile(const std::string& filepath, 
                                    std::string& header,
                                    std::vector<uint8_t>& compressed_data, 
                                    Algorithm& algo,
                                    OperationPoint& op_point) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;

    // Magic number
    char magic[6] = {0};
    file.read(magic, 5);
    if (std::string(magic) != "STCMP") return false;

    // Version
    uint8_t version;
    file.read(reinterpret_cast<char*>(&version), 1);
    if (version == 1) {
        // Old format compatibility
        uint8_t op;
        file.read(reinterpret_cast<char*>(&op), 1);
        algo = Algorithm::ZSTD;  // Old format only supported ZSTD
        op_point = static_cast<OperationPoint>(op);
    } else if (version == 2) {
        // New format
        uint8_t algo_byte;
        file.read(reinterpret_cast<char*>(&algo_byte), 1);
        algo = static_cast<Algorithm>(algo_byte);
        
        uint8_t op;
        file.read(reinterpret_cast<char*>(&op), 1);
        op_point = static_cast<OperationPoint>(op);
    } else {
        return false;
    }

    // Header
    uint64_t header_size;
    file.read(reinterpret_cast<char*>(&header_size), 8);
    std::vector<char> header_buf(header_size);
    file.read(header_buf.data(), header_size);
    header = std::string(header_buf.begin(), header_buf.end());

    // Compressed data
    uint64_t compressed_size;
    file.read(reinterpret_cast<char*>(&compressed_size), 8);
    compressed_data.resize(compressed_size);
    file.read(reinterpret_cast<char*>(compressed_data.data()), compressed_size);

    file.close();
    return true;
}
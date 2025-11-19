#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <map>
#include "../includes/safetensors_parser.hpp"
#include "../includes/compressor.hpp"
#include "../includes/benchmarker.hpp"

void printUsage(const char* prog) {
    std::cout << "SafeTensors Compressor - Enhanced Multi-Algorithm Version\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << prog << " compress <input.safetensors> <output.stcmp> [algorithm] [mode]\n";
    std::cout << "  " << prog << " decompress <input.stcmp> <output.safetensors>\n";
    std::cout << "  " << prog << " benchmark <input.safetensors> [mode]\n";
    std::cout << "  " << prog << " compare <input.safetensors> [mode]\n\n";
    std::cout << "Algorithms:\n";
    std::cout << "  lz4      - LZ4 (fastest, lower ratio)\n";
    std::cout << "  deflate  - DEFLATE/GZIP (good balance)\n";
    std::cout << "  zstd     - Zstandard (best balance) [default]\n";
    std::cout << "  lzma     - LZMA/XZ (highest ratio, slowest)\n\n";
    std::cout << "Modes:\n";
    std::cout << "  fast     - Quick compression\n";
    std::cout << "  balanced - Balance speed/ratio [default]\n";
    std::cout << "  maximum  - Maximum compression\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << prog << " compress model.safetensors model.stcmp zstd balanced\n";
    std::cout << "  " << prog << " benchmark model.safetensors\n";
    std::cout << "  " << prog << " compare model.safetensors balanced\n";
}

Compressor::Algorithm parseAlgorithm(const std::string& algo_str) {
    static std::map<std::string, Compressor::Algorithm> algo_map = {
        {"lz4", Compressor::Algorithm::LZ4},
        {"deflate", Compressor::Algorithm::DEFLATE},
        {"zstd", Compressor::Algorithm::ZSTD},
        {"lzma", Compressor::Algorithm::LZMA}
    };
    
    auto it = algo_map.find(algo_str);
    if (it != algo_map.end()) {
        return it->second;
    }
    return Compressor::Algorithm::ZSTD; // Default
}

Compressor::OperationPoint parseMode(const std::string& mode_str) {
    if (mode_str == "fast") return Compressor::OperationPoint::FAST;
    if (mode_str == "maximum") return Compressor::OperationPoint::MAXIMUM;
    return Compressor::OperationPoint::BALANCED; // Default
}

int compress(const std::string& input, const std::string& output, 
             const std::string& algo_str, const std::string& mode_str) {
    Compressor::Algorithm algo = parseAlgorithm(algo_str);
    Compressor::OperationPoint mode = parseMode(mode_str);

    SafetensorsParser parser;
    if (!parser.parse(input)) return 1;

    Compressor compressor;
    std::cout << "\nCompressing with " << Compressor::getAlgorithmName(algo) 
              << " (" << Compressor::getOperationPointName(mode) << ")..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<uint8_t> compressed = compressor.compress(parser.getTensorData(), algo, mode);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    if (!compressor.writeCompressedFile(output, parser.getHeader(), compressed, algo, mode)) {
        std::cerr << "Error: Failed to write compressed file" << std::endl;
        return 1;
    }

    size_t orig = parser.getTensorDataSize();
    size_t comp = compressed.size();
    double ratio = static_cast<double>(orig) / comp;
    double savings = 100.0 * (1.0 - 1.0/ratio);
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "COMPRESSION COMPLETE" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << "Algorithm:      " << Compressor::getAlgorithmName(algo) << std::endl;
    std::cout << "Mode:           " << Compressor::getOperationPointName(mode) << std::endl;
    std::cout << "Original:       " << std::fixed << std::setprecision(2) 
              << (orig / 1024.0 / 1024.0) << " MB" << std::endl;
    std::cout << "Compressed:     " << (comp / 1024.0 / 1024.0) << " MB" << std::endl;
    std::cout << "Ratio:          " << std::setprecision(3) << ratio << "x" << std::endl;
    std::cout << "Space saved:    " << std::setprecision(1) << savings << "%" << std::endl;
    std::cout << "Time:           " << std::setprecision(2) << duration.count() << " s" << std::endl;
    std::cout << "Throughput:     " << std::setprecision(1) 
              << (orig / 1024.0 / 1024.0 / duration.count()) << " MB/s" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << "\nSuccess: " << output << std::endl;
    return 0;
}

int decompress(const std::string& input, const std::string& output) {
    Compressor compressor;
    std::string header;
    std::vector<uint8_t> compressed;
    Compressor::Algorithm algo;
    Compressor::OperationPoint mode;

    if (!compressor.readCompressedFile(input, header, compressed, algo, mode)) {
        std::cerr << "Error: Failed to read compressed file" << std::endl;
        return 1;
    }

    std::cout << "\nDecompressing " << Compressor::getAlgorithmName(algo) 
              << " data..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<uint8_t> tensor_data = compressor.decompress(compressed, algo, mode);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::ofstream file(output, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot create output file" << std::endl;
        return 1;
    }

    uint64_t header_size = header.size();
    file.write(reinterpret_cast<const char*>(&header_size), 8);
    file.write(header.data(), header.size());
    file.write(reinterpret_cast<const char*>(tensor_data.data()), tensor_data.size());
    file.close();

    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "DECOMPRESSION COMPLETE" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << "Algorithm:      " << Compressor::getAlgorithmName(algo) << std::endl;
    std::cout << "Size:           " << std::fixed << std::setprecision(2) 
              << (tensor_data.size() / 1024.0 / 1024.0) << " MB" << std::endl;
    std::cout << "Time:           " << std::setprecision(2) << duration.count() << " s" << std::endl;
    std::cout << "Throughput:     " << std::setprecision(1) 
              << (tensor_data.size() / 1024.0 / 1024.0 / duration.count()) << " MB/s" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << "\nSuccess: " << output << std::endl;
    return 0;
}

int benchmark(const std::string& input, const std::string& mode_str) {
    SafetensorsParser parser;
    if (!parser.parse(input)) return 1;

    Benchmarker benchmarker;
    std::vector<Benchmarker::BenchmarkResult> results;
    
    if (mode_str.empty()) {
        // Full benchmark - all algorithms, all modes
        results = benchmarker.runAllBenchmarks(parser.getTensorData());
    } else {
        // Specific mode benchmark
        Compressor::OperationPoint mode = parseMode(mode_str);
        results = benchmarker.runAlgorithmComparison(parser.getTensorData(), mode);
    }

    benchmarker.printComparisonTable(results);
    benchmarker.printResults(results);
    benchmarker.saveResultsJSON(results, "output/benchmark_results.json");
    benchmarker.saveResultsCSV(results, "output/benchmark_results.csv");

    return 0;
}

int compare(const std::string& input, const std::string& mode_str) {
    SafetensorsParser parser;
    if (!parser.parse(input)) return 1;

    Compressor::OperationPoint mode = parseMode(mode_str.empty() ? "balanced" : mode_str);
    
    Benchmarker benchmarker;
    std::vector<Benchmarker::BenchmarkResult> results = 
        benchmarker.runAlgorithmComparison(parser.getTensorData(), mode);

    benchmarker.printComparisonTable(results);
    benchmarker.saveResultsJSON(results, "output/comparison_results.json");
    benchmarker.saveResultsCSV(results, "output/comparison_results.csv");

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "compress" && argc >= 4) {
        std::string algo = (argc >= 5) ? argv[4] : "zstd";
        std::string mode = (argc >= 6) ? argv[5] : "balanced";
        return compress(argv[2], argv[3], algo, mode);
    } 
    else if (cmd == "decompress" && argc >= 4) {
        return decompress(argv[2], argv[3]);
    } 
    else if (cmd == "benchmark" && argc >= 3) {
        std::string mode = (argc >= 4) ? argv[3] : "";
        return benchmark(argv[2], mode);
    }
    else if (cmd == "compare" && argc >= 3) {
        std::string mode = (argc >= 4) ? argv[3] : "balanced";
        return compare(argv[2], mode);
    }

    printUsage(argv[0]);
    return 1;
}
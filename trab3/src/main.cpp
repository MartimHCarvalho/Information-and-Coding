#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include "safetensors_parser.hpp"
#include "compressor.hpp"
#include "benchmarker.hpp"

void printUsage(const char* prog) {
    std::cout << "SafeTensors Compressor - Information and Coding Lab 3\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << prog << " compress <input.safetensors> <output.stcmp> [fast|balanced|maximum]\n";
    std::cout << "  " << prog << " decompress <input.stcmp> <output.safetensors>\n";
    std::cout << "  " << prog << " benchmark <input.safetensors>\n\n";
    std::cout << "Modes:\n";
    std::cout << "  fast     - Byte reorder + ZSTD-10 (~1.4x, <30s)\n";
    std::cout << "  balanced - Delta + ZSTD-15 (~2.0x, ~60s) [default]\n";
    std::cout << "  maximum  - Combined + LZMA-9 (~2.3x, >120s)\n";
}

int compress(const std::string& input, const std::string& output, const std::string& mode_str) {
    Compressor::OperationPoint mode = Compressor::OperationPoint::BALANCED;
    if (mode_str == "fast") mode = Compressor::OperationPoint::FAST;
    else if (mode_str == "maximum") mode = Compressor::OperationPoint::MAXIMUM;

    SafetensorsParser parser;
    if (!parser.parse(input)) return 1;

    Compressor compressor;
    std::cout << "\nCompressing (" << Compressor::getOperationPointName(mode) << ")..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<uint8_t> compressed = compressor.compress(parser.getTensorData(), mode);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    if (!compressor.writeCompressedFile(output, parser.getHeader(), compressed, mode)) return 1;

    size_t orig = parser.getTensorDataSize();
    size_t comp = compressed.size();
    std::cout << "\nOriginal: " << (orig / 1024.0 / 1024.0) << " MB" << std::endl;
    std::cout << "Compressed: " << (comp / 1024.0 / 1024.0) << " MB" << std::endl;
    std::cout << "Ratio: " << std::fixed << std::setprecision(2)
              << (static_cast<double>(orig) / comp) << "x" << std::endl;
    std::cout << "Time: " << std::setprecision(1) << duration.count() << "s" << std::endl;
    std::cout << "\nSuccess: " << output << std::endl;
    return 0;
}

int decompress(const std::string& input, const std::string& output) {
    Compressor compressor;
    std::string header;
    std::vector<uint8_t> compressed;
    Compressor::OperationPoint mode;

    if (!compressor.readCompressedFile(input, header, compressed, mode)) return 1;

    std::cout << "\nDecompressing (" << Compressor::getOperationPointName(mode) << ")..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<uint8_t> tensor_data = compressor.decompress(compressed, mode);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::ofstream file(output, std::ios::binary);
    if (!file.is_open()) return 1;

    uint64_t header_size = header.size();
    file.write(reinterpret_cast<const char*>(&header_size), 8);
    file.write(header.data(), header.size());
    file.write(reinterpret_cast<const char*>(tensor_data.data()), tensor_data.size());
    file.close();

    std::cout << "Size: " << (tensor_data.size() / 1024.0 / 1024.0) << " MB" << std::endl;
    std::cout << "Time: " << std::fixed << std::setprecision(1) << duration.count() << "s" << std::endl;
    std::cout << "\nSuccess: " << output << std::endl;
    return 0;
}

int benchmark(const std::string& input) {
    SafetensorsParser parser;
    if (!parser.parse(input)) return 1;

    Benchmarker benchmarker;
    std::vector<Benchmarker::BenchmarkResult> results = benchmarker.runAllBenchmarks(parser.getTensorData());

    benchmarker.printResults(results);
    benchmarker.saveResultsJSON(results, "output/benchmark_results.json");
    benchmarker.saveResultsCSV(results, "output/benchmark_results.csv");

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "compress" && argc >= 4) {
        std::string mode = (argc >= 5) ? argv[4] : "balanced";
        return compress(argv[2], argv[3], mode);
    } else if (cmd == "decompress" && argc >= 4) {
        return decompress(argv[2], argv[3]);
    } else if (cmd == "benchmark" && argc >= 3) {
        return benchmark(argv[2]);
    }

    printUsage(argv[0]);
    return 1;
}

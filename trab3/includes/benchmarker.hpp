#ifndef BENCHMARKER_HPP
#define BENCHMARKER_HPP

#include <string>
#include <vector>
#include <chrono>
#include "compressor.hpp"

class Benchmarker {
public:
    struct BenchmarkResult {
        std::string algorithm;
        std::string operation_point;
        std::string preprocessing;
        size_t original_size;
        size_t compressed_size;
        double compression_ratio;
        double preprocess_time;
        double compress_time;
        double decompress_time;
        double deprocess_time;
        double total_compress_time;
        double total_decompress_time;
        double throughput_mb_per_sec;
        double original_entropy;
        double preprocessed_entropy;
        double entropy_reduction;
        double peak_memory_mb;
        bool decompression_verified;
    };

    Benchmarker() = default;
    ~Benchmarker() = default;

    BenchmarkResult runBenchmark(const std::vector<uint8_t>& data, 
                                 Compressor::Algorithm algo,
                                 Compressor::OperationPoint op_point);
    
    std::vector<BenchmarkResult> runAllBenchmarks(const std::vector<uint8_t>& data);
    
    std::vector<BenchmarkResult> runAlgorithmComparison(const std::vector<uint8_t>& data,
                                                        Compressor::OperationPoint op_point);

    void printResults(const std::vector<BenchmarkResult>& results);
    void printComparisonTable(const std::vector<BenchmarkResult>& results);
    bool saveResultsJSON(const std::vector<BenchmarkResult>& results, const std::string& filepath);
    bool saveResultsCSV(const std::vector<BenchmarkResult>& results, const std::string& filepath);

private:
    Compressor compressor_;

    class Timer {
    public:
        void start() { start_ = std::chrono::high_resolution_clock::now(); }
        double stop() {
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> diff = end - start_;
            return diff.count();
        }
    private:
        std::chrono::high_resolution_clock::time_point start_;
    };

    double getPeakMemoryUsageMB();
    bool verifyDecompression(const std::vector<uint8_t>& original, 
                            const std::vector<uint8_t>& decompressed);
};

#endif
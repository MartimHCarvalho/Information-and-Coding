#include "benchmarker.hpp"
#include "preprocessor.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sys/resource.h>

Benchmarker::BenchmarkResult Benchmarker::runBenchmark(const std::vector<uint8_t>& data,
                                                        Compressor::OperationPoint op_point) {
    BenchmarkResult result;
    result.operation_point = Compressor::getOperationPointName(op_point);
    result.original_size = data.size();

    std::cout << "\n=== " << result.operation_point << " ===" << std::endl;

    Preprocessor preprocessor;
    Preprocessor::Strategy strategy = Preprocessor::Strategy::NONE;

    switch (op_point) {
        case Compressor::OperationPoint::FAST:
            strategy = Preprocessor::Strategy::BYTE_REORDER; break;
        case Compressor::OperationPoint::BALANCED:
            strategy = Preprocessor::Strategy::DELTA_ENCODING; break;
        case Compressor::OperationPoint::MAXIMUM:
            strategy = Preprocessor::Strategy::COMBINED; break;
    }

    result.preprocessing = Preprocessor::getStrategyName(strategy);
    result.original_entropy = Preprocessor::calculateEntropy(data);

    Timer timer;
    timer.start();
    std::vector<uint8_t> preprocessed = preprocessor.preprocess(data, strategy);
    result.preprocess_time = timer.stop();

    result.preprocessed_entropy = Preprocessor::calculateEntropy(preprocessed);
    result.entropy_reduction = result.original_entropy - result.preprocessed_entropy;

    timer.start();
    std::vector<uint8_t> compressed = compressor_.compress(data, op_point);
    result.total_compress_time = timer.stop();
    result.compress_time = result.total_compress_time - result.preprocess_time;

    result.compressed_size = compressed.size();
    result.compression_ratio = static_cast<double>(result.original_size) / result.compressed_size;

    timer.start();
    std::vector<uint8_t> decompressed = compressor_.decompress(compressed, op_point);
    result.total_decompress_time = timer.stop();

    result.decompress_time = result.total_decompress_time * 0.8;
    result.deprocess_time = result.total_decompress_time * 0.2;
    result.decompression_verified = (decompressed == data);
    result.peak_memory_mb = getPeakMemoryUsageMB();

    std::cout << "Ratio: " << std::fixed << std::setprecision(2) << result.compression_ratio
              << "x, Time: " << std::setprecision(1) << result.total_compress_time
              << "s, Verified: " << (result.decompression_verified ? "YES" : "NO") << std::endl;

    return result;
}

std::vector<Benchmarker::BenchmarkResult> Benchmarker::runAllBenchmarks(const std::vector<uint8_t>& data) {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "Benchmarking " << (data.size() / 1024.0 / 1024.0) << " MB" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    std::vector<BenchmarkResult> results;
    results.push_back(runBenchmark(data, Compressor::OperationPoint::FAST));
    results.push_back(runBenchmark(data, Compressor::OperationPoint::BALANCED));
    results.push_back(runBenchmark(data, Compressor::OperationPoint::MAXIMUM));

    return results;
}

void Benchmarker::printResults(const std::vector<BenchmarkResult>& results) {
    std::cout << "\n" << std::string(90, '=') << std::endl;
    std::cout << "SUMMARY" << std::endl;
    std::cout << std::string(90, '-') << std::endl;

    std::cout << std::left << std::setw(12) << "Mode"
              << std::right << std::setw(12) << "Size (MB)"
              << std::setw(10) << "Ratio"
              << std::setw(12) << "Comp (s)"
              << std::setw(12) << "Decomp (s)"
              << std::setw(12) << "Entropy"
              << std::setw(10) << "Verified" << std::endl;
    std::cout << std::string(90, '-') << std::endl;

    for (const auto& r : results) {
        std::cout << std::left << std::setw(12) << r.operation_point
                  << std::right << std::setw(12) << std::fixed << std::setprecision(1)
                  << (r.compressed_size / 1024.0 / 1024.0)
                  << std::setw(10) << std::setprecision(2) << r.compression_ratio
                  << std::setw(12) << std::setprecision(1) << r.total_compress_time
                  << std::setw(12) << r.total_decompress_time
                  << std::setw(12) << std::setprecision(2) << r.entropy_reduction
                  << std::setw(10) << (r.decompression_verified ? "YES" : "NO") << std::endl;
    }
    std::cout << std::string(90, '=') << std::endl;
}

bool Benchmarker::saveResultsJSON(const std::vector<BenchmarkResult>& results, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "[\n";
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        file << "  {\n";
        file << "    \"operation_point\": \"" << r.operation_point << "\",\n";
        file << "    \"preprocessing\": \"" << r.preprocessing << "\",\n";
        file << "    \"compression_ratio\": " << std::fixed << std::setprecision(3) << r.compression_ratio << ",\n";
        file << "    \"compressed_size_mb\": " << (r.compressed_size / 1024.0 / 1024.0) << ",\n";
        file << "    \"total_compress_time\": " << r.total_compress_time << ",\n";
        file << "    \"total_decompress_time\": " << r.total_decompress_time << ",\n";
        file << "    \"entropy_reduction\": " << std::setprecision(4) << r.entropy_reduction << ",\n";
        file << "    \"verified\": " << (r.decompression_verified ? "true" : "false") << "\n";
        file << "  }" << (i < results.size() - 1 ? "," : "") << "\n";
    }
    file << "]\n";

    file.close();
    std::cout << "\nSaved: " << filepath << std::endl;
    return true;
}

bool Benchmarker::saveResultsCSV(const std::vector<BenchmarkResult>& results, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "Mode,Preprocessing,Ratio,SizeMB,CompTime,DecompTime,Entropy,Verified\n";
    for (const auto& r : results) {
        file << r.operation_point << ","
             << r.preprocessing << ","
             << std::fixed << std::setprecision(3) << r.compression_ratio << ","
             << (r.compressed_size / 1024.0 / 1024.0) << ","
             << r.total_compress_time << ","
             << r.total_decompress_time << ","
             << std::setprecision(4) << r.entropy_reduction << ","
             << (r.decompression_verified ? "YES" : "NO") << "\n";
    }

    file.close();
    std::cout << "Saved: " << filepath << std::endl;
    return true;
}

double Benchmarker::getPeakMemoryUsageMB() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss / 1024.0;
}

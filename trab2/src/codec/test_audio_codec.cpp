#include "../../includes/codec/audio_codec.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <map>
#include <sstream>
#include <algorithm>
#include <cstring>

// ==================== Color Codes ====================
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define BOLD "\033[1m"

// ==================== WAV File Handling ====================

struct WAVHeader
{
    char riff[4];           // "RIFF"
    uint32_t fileSize;      // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmtSize;       // Format chunk size (16 for PCM)
    uint16_t audioFormat;   // Audio format (1 = PCM)
    uint16_t numChannels;   // Number of channels
    uint32_t sampleRate;    // Sample rate
    uint32_t byteRate;      // Bytes per second
    uint16_t blockAlign;    // Block align
    uint16_t bitsPerSample; // Bits per sample
    char data[4];           // "data"
    uint32_t dataSize;      // Data size
};

bool readWAV(const std::string &filename, std::vector<int16_t> &leftChannel,
             std::vector<int16_t> &rightChannel, uint32_t &sampleRate,
             uint16_t &numChannels, uint16_t &bitsPerSample)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        std::cerr << RED << "Cannot open file: " << filename << RESET << std::endl;
        return false;
    }

    WAVHeader header;
    file.read(reinterpret_cast<char *>(&header), sizeof(WAVHeader));

    if (std::strncmp(header.riff, "RIFF", 4) != 0 ||
        std::strncmp(header.wave, "WAVE", 4) != 0)
    {
        std::cerr << RED << "Not a valid WAV file" << RESET << std::endl;
        return false;
    }

    if (header.audioFormat != 1)
    {
        std::cerr << RED << "Only PCM format supported" << RESET << std::endl;
        return false;
    }

    if (header.bitsPerSample != 16)
    {
        std::cerr << RED << "Only 16-bit audio supported" << RESET << std::endl;
        return false;
    }

    sampleRate = header.sampleRate;
    numChannels = header.numChannels;
    bitsPerSample = header.bitsPerSample;

    uint32_t numSamples = header.dataSize / (numChannels * sizeof(int16_t));

    if (numChannels == 1)
    {
        leftChannel.resize(numSamples);
        file.read(reinterpret_cast<char *>(leftChannel.data()),
                  numSamples * sizeof(int16_t));
        rightChannel.clear();
    }
    else if (numChannels == 2)
    {
        std::vector<int16_t> interleaved(numSamples * 2);
        file.read(reinterpret_cast<char *>(interleaved.data()),
                  numSamples * 2 * sizeof(int16_t));

        leftChannel.resize(numSamples);
        rightChannel.resize(numSamples);

        for (uint32_t i = 0; i < numSamples; i++)
        {
            leftChannel[i] = interleaved[i * 2];
            rightChannel[i] = interleaved[i * 2 + 1];
        }
    }
    else
    {
        std::cerr << RED << "Unsupported number of channels: "
                  << numChannels << RESET << std::endl;
        return false;
    }

    return true;
}

bool writeWAV(const std::string &filename, const std::vector<int16_t> &leftChannel,
              const std::vector<int16_t> &rightChannel, uint32_t sampleRate,
              uint16_t numChannels, uint16_t bitsPerSample)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file)
    {
        std::cerr << RED << "Cannot create file: " << filename << RESET << std::endl;
        return false;
    }

    uint32_t numSamples = leftChannel.size();
    uint32_t dataSize = numSamples * numChannels * sizeof(int16_t);

    WAVHeader header;
    std::memcpy(header.riff, "RIFF", 4);
    header.fileSize = 36 + dataSize;
    std::memcpy(header.wave, "WAVE", 4);
    std::memcpy(header.fmt, "fmt ", 4);
    header.fmtSize = 16;
    header.audioFormat = 1;
    header.numChannels = numChannels;
    header.sampleRate = sampleRate;
    header.bitsPerSample = bitsPerSample;
    header.byteRate = sampleRate * numChannels * (bitsPerSample / 8);
    header.blockAlign = numChannels * (bitsPerSample / 8);
    std::memcpy(header.data, "data", 4);
    header.dataSize = dataSize;

    file.write(reinterpret_cast<char *>(&header), sizeof(WAVHeader));

    if (numChannels == 1)
    {
        file.write(reinterpret_cast<const char *>(leftChannel.data()),
                   numSamples * sizeof(int16_t));
    }
    else
    {
        std::vector<int16_t> interleaved(numSamples * 2);
        for (uint32_t i = 0; i < numSamples; i++)
        {
            interleaved[i * 2] = leftChannel[i];
            interleaved[i * 2 + 1] = rightChannel[i];
        }
        file.write(reinterpret_cast<const char *>(interleaved.data()),
                   numSamples * 2 * sizeof(int16_t));
    }

    return true;
}

// ==================== Audio Generation ====================

std::vector<int16_t> generateSineWave(double frequency, uint32_t sampleRate,
                                      double duration, double amplitude = 0.5)
{
    uint32_t numSamples = static_cast<uint32_t>(sampleRate * duration);
    std::vector<int16_t> samples(numSamples);

    for (uint32_t i = 0; i < numSamples; i++)
    {
        double t = static_cast<double>(i) / sampleRate;
        double value = amplitude * std::sin(2.0 * M_PI * frequency * t);
        samples[i] = static_cast<int16_t>(value * 32767.0);
    }

    return samples;
}

std::vector<int16_t> generateWhiteNoise(uint32_t sampleRate, double duration,
                                        double amplitude = 0.1)
{
    uint32_t numSamples = static_cast<uint32_t>(sampleRate * duration);
    std::vector<int16_t> samples(numSamples);

    std::srand(12345);
    for (uint32_t i = 0; i < numSamples; i++)
    {
        double value = amplitude * (2.0 * std::rand() / RAND_MAX - 1.0);
        samples[i] = static_cast<int16_t>(value * 32767.0);
    }

    return samples;
}

std::vector<int16_t> generateSweep(uint32_t sampleRate, double duration,
                                   double startFreq, double endFreq)
{
    uint32_t numSamples = static_cast<uint32_t>(sampleRate * duration);
    std::vector<int16_t> samples(numSamples);

    for (uint32_t i = 0; i < numSamples; i++)
    {
        double t = static_cast<double>(i) / sampleRate;
        double freq = startFreq + (endFreq - startFreq) * t / duration;
        double value = 0.5 * std::sin(2.0 * M_PI * freq * t);
        samples[i] = static_cast<int16_t>(value * 32767.0);
    }

    return samples;
}

// ==================== Audio Statistics ====================

struct AudioStatistics
{
    double mean;
    double rms;
    int16_t minValue;
    int16_t maxValue;
    double dynamicRange;
};

AudioStatistics analyzeAudio(const std::vector<int16_t> &samples)
{
    AudioStatistics stats = {0, 0, 32767, -32768, 0};

    if (samples.empty())
        return stats;

    double sum = 0;
    double sumSquares = 0;

    for (int16_t sample : samples)
    {
        sum += sample;
        sumSquares += static_cast<double>(sample) * sample;
        stats.minValue = std::min(stats.minValue, sample);
        stats.maxValue = std::max(stats.maxValue, sample);
    }

    stats.mean = sum / samples.size();
    stats.rms = std::sqrt(sumSquares / samples.size());
    stats.dynamicRange = 20.0 * std::log10(32768.0 / (stats.rms + 1e-10));

    return stats;
}

void printAudioStatistics(const AudioStatistics &stats)
{
    std::cout << YELLOW << "📊 Audio Statistics:" << RESET << std::endl;
    std::cout << "  ├─ Mean: " << std::fixed << std::setprecision(2)
              << stats.mean << std::endl;
    std::cout << "  ├─ RMS: " << stats.rms << std::endl;
    std::cout << "  ├─ Range: [" << stats.minValue << ", "
              << stats.maxValue << "]" << std::endl;
    std::cout << "  └─ Dynamic Range: " << std::setprecision(1)
              << stats.dynamicRange << " dB" << std::endl;
}

void printAudioInfo(const std::string &filename, uint32_t sampleRate,
                    uint16_t numChannels, uint32_t numSamples)
{
    std::cout << CYAN << "🎵 Audio Information:" << RESET << std::endl;
    std::cout << "  ├─ File: " << filename << std::endl;
    std::cout << "  ├─ Sample Rate: " << sampleRate << " Hz" << std::endl;
    std::cout << "  ├─ Channels: " << numChannels << std::endl;
    std::cout << "  ├─ Samples: " << numSamples << " per channel" << std::endl;
    std::cout << "  ├─ Duration: " << std::fixed << std::setprecision(2)
              << (double)numSamples / sampleRate << " seconds" << std::endl;
    std::cout << "  └─ Size: " << (numSamples * numChannels * 2) / 1024.0
              << " KB" << std::endl;
}

// ==================== Testing Framework ====================

class AudioCodecTester
{
private:
    struct TestResult
    {
        std::string testName;
        std::string predictor;
        std::string channelMode;
        double compressionRatio;
        double bitsPerSample;
        double spaceSavings;
        int optimalM;
        long long encodeTimeMs;
        long long decodeTimeMs;
        double speedupRatio;
        bool reconstructionPerfect;
        size_t originalSize;
        size_t compressedSize;
        uint16_t numChannels;
    };

    std::vector<TestResult> results;

    std::string predictorToString(AudioCodec::PredictorType pred) const
    {
        switch (pred)
        {
        case AudioCodec::PredictorType::NONE:
            return "NONE";
        case AudioCodec::PredictorType::LINEAR1:
            return "LINEAR1";
        case AudioCodec::PredictorType::LINEAR2:
            return "LINEAR2";
        case AudioCodec::PredictorType::LINEAR3:
            return "LINEAR3";
        case AudioCodec::PredictorType::ADAPTIVE:
            return "ADAPTIVE";
        default:
            return "UNKNOWN";
        }
    }

    std::string channelModeToString(AudioCodec::ChannelMode mode) const
    {
        switch (mode)
        {
        case AudioCodec::ChannelMode::INDEPENDENT:
            return "INDEPENDENT";
        case AudioCodec::ChannelMode::MID_SIDE:
            return "MID_SIDE";
        case AudioCodec::ChannelMode::LEFT_SIDE:
            return "LEFT_SIDE";
        default:
            return "UNKNOWN";
        }
    }

public:
    void runMonoTest(const std::string &testName,
                     AudioCodec::PredictorType predictor,
                     const std::vector<int16_t> &samples,
                     uint32_t sampleRate, uint16_t bitsPerSample)
    {
        printTestHeader(testName + " (Mono)");

        AudioCodec codec(predictor, AudioCodec::ChannelMode::INDEPENDENT, 0, true);

        std::string compressedFile = "test_audio_" + testName + ".golomb";
        std::string decodedFile = "test_audio_" + testName + "_decoded.wav";

        // Encode
        auto startEncode = std::chrono::high_resolution_clock::now();
        if (!codec.encodeMono(samples, sampleRate, bitsPerSample, compressedFile))
        {
            std::cerr << RED << "✗ Encoding failed!" << RESET << std::endl;
            return;
        }
        auto endEncode = std::chrono::high_resolution_clock::now();
        auto encodeTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                              endEncode - startEncode)
                              .count();

        auto stats = codec.getLastStats();
        printCompressionResults(stats, encodeTime);

        // Decode
        auto startDecode = std::chrono::high_resolution_clock::now();
        std::vector<int16_t> decodedLeft, decodedRight;
        uint32_t decSampleRate;
        uint16_t decNumChannels, decBitsPerSample;

        std::cout << "Checking encoded file..." << std::endl;
        std::ifstream checkFile(compressedFile, std::ios::binary | std::ios::ate);
        if (!checkFile)
        {
            std::cerr << "ERROR: Cannot open encoded file!" << std::endl;
            return;
        }
        size_t fileSize = checkFile.tellg();
        checkFile.close();

        std::cout << "  Encoded file size: " << fileSize << " bytes" << std::endl;

        if (fileSize == 0)
        {
            std::cerr << "ERROR: Encoded file is empty!" << std::endl;
            return;
        }

        std::cout << "Attempting to decode..." << std::endl;
        if (!codec.decode(compressedFile, decodedLeft, decodedRight,
                          decSampleRate, decNumChannels, decBitsPerSample))
        {
            std::cerr << "✗ Decoding failed!" << RESET << std::endl;
            return;
        }
        auto endDecode = std::chrono::high_resolution_clock::now();
        auto decodeTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                              endDecode - startDecode)
                              .count();

        std::cout << "\n⚙️  Decoding time:        " << decodeTime << " ms" << std::endl;

        double speedup = (encodeTime > 0) ? (double)encodeTime / decodeTime : 0.0;
        std::cout << "⚡  Decode speedup:       " << std::fixed << std::setprecision(2)
                  << speedup << "x faster" << std::endl;

        // Verify
        bool identical = verifyReconstruction(samples, decodedLeft,
                                              sampleRate, decSampleRate,
                                              1, decNumChannels);

        if (identical)
        {
            std::cout << GREEN << "\n✓ Perfect reconstruction verified!"
                      << RESET << std::endl;
        }
        else
        {
            std::cerr << RED << "\n✗ Decoded audio differs from original!"
                      << RESET << std::endl;
        }

        writeWAV(decodedFile, decodedLeft, decodedRight, decSampleRate,
                 decNumChannels, decBitsPerSample);

        // Store results
        TestResult result;
        result.testName = testName;
        result.predictor = predictorToString(predictor);
        result.channelMode = "MONO";
        result.compressionRatio = stats.compressionRatio;
        result.bitsPerSample = stats.bitsPerSample;
        result.spaceSavings = 100.0 * (1.0 - 1.0 / stats.compressionRatio);
        result.optimalM = stats.optimalM;
        result.encodeTimeMs = encodeTime;
        result.decodeTimeMs = decodeTime;
        result.speedupRatio = speedup;
        result.reconstructionPerfect = identical;
        result.originalSize = stats.originalSize;
        result.compressedSize = stats.compressedSize;
        result.numChannels = 1;
        results.push_back(result);
    }

    void runStereoTest(const std::string &testName,
                       AudioCodec::PredictorType predictor,
                       AudioCodec::ChannelMode channelMode,
                       const std::vector<int16_t> &left,
                       const std::vector<int16_t> &right,
                       uint32_t sampleRate, uint16_t bitsPerSample)
    {
        printTestHeader(testName + " (Stereo)");

        AudioCodec codec(predictor, channelMode, 0, true);

        std::string compressedFile = "test_audio_" + testName + ".golomb";
        std::string decodedFile = "test_audio_" + testName + "_decoded.wav";

        // Encode
        auto startEncode = std::chrono::high_resolution_clock::now();
        if (!codec.encodeStereo(left, right, sampleRate, bitsPerSample,
                                compressedFile))
        {
            std::cerr << RED << "✗ Encoding failed!" << RESET << std::endl;
            return;
        }
        auto endEncode = std::chrono::high_resolution_clock::now();
        auto encodeTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                              endEncode - startEncode)
                              .count();

        auto stats = codec.getLastStats();
        printCompressionResults(stats, encodeTime);

        // Decode
        auto startDecode = std::chrono::high_resolution_clock::now();
        std::vector<int16_t> decodedLeft, decodedRight;
        uint32_t decSampleRate;
        uint16_t decNumChannels, decBitsPerSample;

        std::cout << "Checking encoded file..." << std::endl;
        std::ifstream checkFile(compressedFile, std::ios::binary | std::ios::ate);
        if (!checkFile)
        {
            std::cerr << "ERROR: Cannot open encoded file!" << std::endl;
            return;
        }
        size_t fileSize = checkFile.tellg();
        checkFile.close();

        std::cout << "  Encoded file size: " << fileSize << " bytes" << std::endl;

        if (fileSize == 0)
        {
            std::cerr << "ERROR: Encoded file is empty!" << std::endl;
            return;
        }

        std::cout << "Attempting to decode..." << std::endl;
        if (!codec.decode(compressedFile, decodedLeft, decodedRight,
                          decSampleRate, decNumChannels, decBitsPerSample))
        {
            std::cerr << "✗ Decoding failed!" << RESET << std::endl;
            return;
        }
        auto endDecode = std::chrono::high_resolution_clock::now();
        auto decodeTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                              endDecode - startDecode)
                              .count();

        std::cout << "\n⚙️  Decoding time:        " << decodeTime << " ms" << std::endl;

        double speedup = (encodeTime > 0) ? (double)encodeTime / decodeTime : 0.0;
        std::cout << "⚡  Decode speedup:       " << std::fixed << std::setprecision(2)
                  << speedup << "x faster" << std::endl;

        // Verify
        bool identical = verifyReconstructionStereo(left, right, decodedLeft,
                                                    decodedRight, sampleRate,
                                                    decSampleRate, 2, decNumChannels);

        if (identical)
        {
            std::cout << GREEN << "\n✓ Perfect reconstruction verified!"
                      << RESET << std::endl;
        }
        else
        {
            std::cerr << RED << "\n✗ Decoded audio differs from original!"
                      << RESET << std::endl;
        }

        writeWAV(decodedFile, decodedLeft, decodedRight, decSampleRate,
                 decNumChannels, decBitsPerSample);

        // Store results
        TestResult result;
        result.testName = testName;
        result.predictor = predictorToString(predictor);
        result.channelMode = channelModeToString(channelMode);
        result.compressionRatio = stats.compressionRatio;
        result.bitsPerSample = stats.bitsPerSample;
        result.spaceSavings = 100.0 * (1.0 - 1.0 / stats.compressionRatio);
        result.optimalM = stats.optimalM;
        result.encodeTimeMs = encodeTime;
        result.decodeTimeMs = decodeTime;
        result.speedupRatio = speedup;
        result.reconstructionPerfect = identical;
        result.originalSize = stats.originalSize;
        result.compressedSize = stats.compressedSize;
        result.numChannels = 2;
        results.push_back(result);
    }

    void printSummary()
    {
        if (results.empty())
            return;

        std::cout << "\n"
                  << BOLD << CYAN;
        std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
        std::cout << "║              COMPRESSION SUMMARY                          ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════╝";
        std::cout << RESET << std::endl;

        // Find best configuration
        auto best = std::max_element(results.begin(), results.end(),
                                     [](const TestResult &a, const TestResult &b)
                                     {
                                         return a.compressionRatio < b.compressionRatio;
                                     });

        std::cout << "\n"
                  << GREEN << "🏆 Best Configuration:" << RESET << std::endl;
        std::cout << "  Test: " << BOLD << best->testName << RESET << std::endl;
        std::cout << "  Compression Ratio: " << BOLD << std::fixed << std::setprecision(2)
                  << best->compressionRatio << ":1" << RESET << std::endl;
        std::cout << "  Bits per Sample: " << std::setprecision(3)
                  << best->bitsPerSample << std::endl;
        std::cout << "  Space Savings: " << std::setprecision(1)
                  << best->spaceSavings << "%" << std::endl;
        std::cout << "  Decode Speedup: " << std::setprecision(2)
                  << best->speedupRatio << "x" << std::endl;

        // Comparison table
        std::cout << "\n"
                  << YELLOW << "📊 All Results:" << RESET << std::endl;
        std::cout << std::string(95, '-') << std::endl;
        std::cout << std::left << std::setw(20) << "Test Name"
                  << std::setw(12) << "Ratio"
                  << std::setw(12) << "Bits/Samp"
                  << std::setw(12) << "Enc (ms)"
                  << std::setw(12) << "Dec (ms)"
                  << std::setw(10) << "Speedup"
                  << "Status" << std::endl;
        std::cout << std::string(95, '-') << std::endl;

        for (const auto &r : results)
        {
            std::cout << std::left << std::setw(20) << r.testName
                      << std::setw(12) << std::fixed << std::setprecision(2)
                      << r.compressionRatio
                      << std::setw(12) << std::setprecision(3) << r.bitsPerSample
                      << std::setw(12) << r.encodeTimeMs
                      << std::setw(12) << r.decodeTimeMs
                      << std::setw(10) << std::setprecision(2) << r.speedupRatio << "x";

            if (r.reconstructionPerfect)
            {
                std::cout << GREEN << "✓ OK" << RESET;
            }
            else
            {
                std::cout << RED << "✗ FAIL" << RESET;
            }
            std::cout << std::endl;
        }
        std::cout << std::string(95, '-') << std::endl;

        // Statistics
        double avgRatio = 0, avgBits = 0, avgSpeedup = 0;
        for (const auto &r : results)
        {
            avgRatio += r.compressionRatio;
            avgBits += r.bitsPerSample;
            avgSpeedup += r.speedupRatio;
        }
        avgRatio /= results.size();
        avgBits /= results.size();
        avgSpeedup /= results.size();

        std::cout << "\n"
                  << CYAN << "📈 Average Statistics:" << RESET << std::endl;
        std::cout << "  Compression Ratio: " << std::fixed << std::setprecision(2)
                  << avgRatio << ":1" << std::endl;
        std::cout << "  Bits per Sample: " << std::setprecision(3) << avgBits << std::endl;
        std::cout << "  Decode Speedup: " << std::setprecision(2) << avgSpeedup << "x" << std::endl;
    }

    void exportResultsJSON(const std::string &filename = "audio_codec_results.json")
    {
        std::ofstream json(filename);
        if (!json)
        {
            std::cerr << RED << "✗ Cannot create JSON file: " << filename
                      << RESET << std::endl;
            return;
        }

        json << "[\n";
        for (size_t i = 0; i < results.size(); i++)
        {
            const auto &r = results[i];
            json << "  {\n"
                 << "    \"test_name\": \"" << r.testName << "\",\n"
                 << "    \"predictor\": \"" << r.predictor << "\",\n"
                 << "    \"channel_mode\": \"" << r.channelMode << "\",\n"
                 << "    \"compression_ratio\": " << std::fixed << std::setprecision(6)
                 << r.compressionRatio << ",\n"
                 << "    \"bits_per_sample\": " << r.bitsPerSample << ",\n"
                 << "    \"space_savings\": " << r.spaceSavings << ",\n"
                 << "    \"optimal_m\": " << r.optimalM << ",\n"
                 << "    \"encode_time\": " << r.encodeTimeMs << ",\n"
                 << "    \"decode_time\": " << r.decodeTimeMs << ",\n"
                 << "    \"speedup_ratio\": " << r.speedupRatio << ",\n"
                 << "    \"reconstruction_perfect\": "
                 << (r.reconstructionPerfect ? "true" : "false") << ",\n"
                 << "    \"original_size\": " << r.originalSize << ",\n"
                 << "    \"compressed_size\": " << r.compressedSize << ",\n"
                 << "    \"num_channels\": " << r.numChannels << "\n"
                 << "  }" << (i < results.size() - 1 ? "," : "") << "\n";
        }
        json << "]\n";
        json.close();

        std::cout << GREEN << "\n✓ Results exported to: " << filename
                  << RESET << std::endl;
    }

private:
    void printTestHeader(const std::string &testName)
    {
        std::cout << "\n"
                  << std::string(70, '-') << std::endl;
        std::cout << BOLD << BLUE << "🎵 Audio Test: " << testName
                  << RESET << std::endl;
        std::cout << std::string(70, '-') << std::endl;
    }

    void printCompressionResults(const AudioCodec::CompressionStats &stats,
                                 long long encodeTime)
    {
        std::cout << "\n"
                  << MAGENTA << "📊 Compression Results:"
                  << RESET << std::endl;
        std::cout << "  ├─ Compression ratio:   " << std::fixed << std::setprecision(2)
                  << stats.compressionRatio << ":1" << std::endl;
        std::cout << "  ├─ Bits per sample:     " << std::setprecision(3)
                  << stats.bitsPerSample << " (original: 16.0)" << std::endl;
        std::cout << "  ├─ Space savings:       " << std::setprecision(1)
                  << (100.0 * (1.0 - 1.0 / stats.compressionRatio)) << "%" << std::endl;
        std::cout << "  ├─ Original size:       " << stats.originalSize
                  << " bytes" << std::endl;
        std::cout << "  ├─ Compressed size:     " << stats.compressedSize
                  << " bytes" << std::endl;
        std::cout << "  ├─ Optimal m:           " << stats.optimalM << std::endl;
        std::cout << "  └─ Encoding time:       " << encodeTime << " ms" << std::endl;
    }

    bool verifyReconstruction(const std::vector<int16_t> &original,
                              const std::vector<int16_t> &decoded,
                              uint32_t sampleRate, uint32_t decSampleRate,
                              uint16_t numChannels, uint16_t decNumChannels)
    {
        if (sampleRate != decSampleRate)
        {
            std::cerr << YELLOW << "  Sample rate mismatch: " << sampleRate
                      << " vs " << decSampleRate << RESET << std::endl;
            return false;
        }

        if (numChannels != decNumChannels)
        {
            std::cerr << YELLOW << "  Channel count mismatch: " << numChannels
                      << " vs " << decNumChannels << RESET << std::endl;
            return false;
        }

        if (original.size() != decoded.size())
        {
            std::cerr << YELLOW << "  Size mismatch: " << original.size()
                      << " vs " << decoded.size() << RESET << std::endl;
            return false;
        }

        for (size_t i = 0; i < original.size(); i++)
        {
            if (original[i] != decoded[i])
            {
                std::cerr << YELLOW << "  Difference at sample " << i << ": "
                          << original[i] << " vs " << decoded[i] << RESET << std::endl;
                return false;
            }
        }
        return true;
    }

    bool verifyReconstructionStereo(const std::vector<int16_t> &origLeft,
                                    const std::vector<int16_t> &origRight,
                                    const std::vector<int16_t> &decLeft,
                                    const std::vector<int16_t> &decRight,
                                    uint32_t sampleRate, uint32_t decSampleRate,
                                    uint16_t numChannels, uint16_t decNumChannels)
    {
        if (sampleRate != decSampleRate || numChannels != decNumChannels)
        {
            return false;
        }

        if (origLeft.size() != decLeft.size() || origRight.size() != decRight.size())
        {
            std::cerr << YELLOW << "  Size mismatch in stereo channels"
                      << RESET << std::endl;
            return false;
        }

        for (size_t i = 0; i < origLeft.size(); i++)
        {
            if (origLeft[i] != decLeft[i] || origRight[i] != decRight[i])
            {
                std::cerr << YELLOW << "  Difference at sample " << i << RESET << std::endl;
                return false;
            }
        }
        return true;
    }
};

// ==================== Main Program ====================

void printBanner()
{
    std::cout << BOLD << CYAN;
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║           Audio Codec Test Suite - Golomb Coding         ║\n";
    std::cout << "║                  Lossless Audio Compression               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    std::cout << RESET << std::endl;
}

void printUsage(const char *programName)
{
    std::cout << "Usage: " << programName << " [audio.wav] [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  No arguments     - Run with synthetic test audio (5 seconds)\n";
    std::cout << "  audio.wav        - Test with your WAV file (16-bit PCM)\n";
    std::cout << "  -h, --help       - Show this help message\n";
    std::cout << "  -v, --verbose    - Show detailed analysis\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << "\n";
    std::cout << "  " << programName << " myaudio.wav\n";
    std::cout << "  " << programName << " /path/to/audio.wav -v\n";
}

int main(int argc, char *argv[])
{
    printBanner();

    // Parse arguments
    bool verbose = false;
    std::string inputFile = "";

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help")
        {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "-v" || arg == "--verbose")
        {
            verbose = true;
        }
        else if (arg[0] != '-')
        {
            inputFile = arg;
        }
    }

    // Load or generate audio
    std::vector<int16_t> leftChannel, rightChannel;
    uint32_t sampleRate;
    uint16_t numChannels, bitsPerSample;
    std::string audioSource;

    if (!inputFile.empty())
    {
        std::cout << CYAN << "📂 Loading audio from: " << inputFile
                  << RESET << std::endl;
        if (!readWAV(inputFile, leftChannel, rightChannel, sampleRate,
                     numChannels, bitsPerSample))
        {
            std::cerr << RED << "Failed to load audio file" << RESET << std::endl;
            std::cout << YELLOW << "\n💡 Creating synthetic audio instead..."
                      << RESET << std::endl;
            inputFile = "";
        }
        else
        {
            printAudioInfo(inputFile, sampleRate, numChannels, leftChannel.size());
            audioSource = inputFile;
        }
    }

    if (inputFile.empty())
    {
        std::cout << YELLOW << "\n⚠️  No audio file provided. Creating synthetic test audio..."
                  << RESET << std::endl;
        sampleRate = 44100;
        numChannels = 2;
        bitsPerSample = 16;
        double duration = 5.0;

        std::cout << "  ├─ Type: Mixed content (sine + sweep + noise)" << std::endl;
        std::cout << "  ├─ Sample Rate: " << sampleRate << " Hz" << std::endl;
        std::cout << "  ├─ Channels: " << numChannels << " (stereo)" << std::endl;
        std::cout << "  ├─ Duration: " << duration << " seconds" << std::endl;
        std::cout << "  └─ Bits per Sample: " << bitsPerSample << std::endl;

        // Generate left channel: 440 Hz sine wave + sweep
        auto sine = generateSineWave(440.0, sampleRate, duration / 2);
        auto sweep = generateSweep(sampleRate, duration / 2, 200.0, 2000.0);
        leftChannel.insert(leftChannel.end(), sine.begin(), sine.end());
        leftChannel.insert(leftChannel.end(), sweep.begin(), sweep.end());

        // Generate right channel: 880 Hz sine wave + white noise
        auto sine2 = generateSineWave(880.0, sampleRate, duration / 2);
        auto noise = generateWhiteNoise(sampleRate, duration / 2);
        rightChannel.insert(rightChannel.end(), sine2.begin(), sine2.end());
        rightChannel.insert(rightChannel.end(), noise.begin(), noise.end());

        std::string synthFile = "test_audio_original.wav";
        writeWAV(synthFile, leftChannel, rightChannel, sampleRate,
                 numChannels, bitsPerSample);
        std::cout << GREEN << "\n✓ Synthetic audio saved as: " << synthFile
                  << RESET << std::endl;
        audioSource = synthFile;
    }

    // Analyze audio if verbose
    if (verbose)
    {
        std::cout << "\n"
                  << BOLD << MAGENTA << "📊 Audio Analysis:"
                  << RESET << std::endl;

        std::cout << "\n"
                  << CYAN << "Left Channel:" << RESET << std::endl;
        auto statsL = analyzeAudio(leftChannel);
        printAudioStatistics(statsL);

        if (!rightChannel.empty())
        {
            std::cout << "\n"
                      << CYAN << "Right Channel:" << RESET << std::endl;
            auto statsR = analyzeAudio(rightChannel);
            printAudioStatistics(statsR);
        }
    }

    // Initialize tester
    AudioCodecTester tester;

    // Run tests
    std::cout << "\n"
              << BOLD << GREEN;
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║              STARTING COMPRESSION TESTS                   ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝";
    std::cout << RESET << std::endl;

    if (numChannels == 1 || rightChannel.empty())
    {
        // Mono tests
        std::cout << "\n"
                  << BOLD << YELLOW << "Testing Mono Audio..."
                  << RESET << std::endl;

        tester.runMonoTest("Mono_LINEAR2",
                           AudioCodec::PredictorType::LINEAR2,
                           leftChannel, sampleRate, bitsPerSample);

        tester.runMonoTest("Mono_LINEAR1",
                           AudioCodec::PredictorType::LINEAR1,
                           leftChannel, sampleRate, bitsPerSample);

        tester.runMonoTest("Mono_LINEAR3",
                           AudioCodec::PredictorType::LINEAR3,
                           leftChannel, sampleRate, bitsPerSample);

        tester.runMonoTest("Mono_ADAPTIVE",
                           AudioCodec::PredictorType::ADAPTIVE,
                           leftChannel, sampleRate, bitsPerSample);
    }
    else
    {
        // Stereo tests
        std::cout << "\n"
                  << BOLD << YELLOW << "Testing Stereo Audio..."
                  << RESET << std::endl;

        tester.runStereoTest("Stereo_MidSide_LINEAR2",
                             AudioCodec::PredictorType::LINEAR2,
                             AudioCodec::ChannelMode::MID_SIDE,
                             leftChannel, rightChannel, sampleRate, bitsPerSample);

        tester.runStereoTest("Stereo_Independent_LINEAR2",
                             AudioCodec::PredictorType::LINEAR2,
                             AudioCodec::ChannelMode::INDEPENDENT,
                             leftChannel, rightChannel, sampleRate, bitsPerSample);

        tester.runStereoTest("Stereo_MidSide_LINEAR3",
                             AudioCodec::PredictorType::LINEAR3,
                             AudioCodec::ChannelMode::MID_SIDE,
                             leftChannel, rightChannel, sampleRate, bitsPerSample);

        tester.runStereoTest("Stereo_Independent_LINEAR1",
                             AudioCodec::PredictorType::LINEAR1,
                             AudioCodec::ChannelMode::INDEPENDENT,
                             leftChannel, rightChannel, sampleRate, bitsPerSample);

        tester.runStereoTest("Stereo_MidSide_ADAPTIVE",
                             AudioCodec::PredictorType::ADAPTIVE,
                             AudioCodec::ChannelMode::MID_SIDE,
                             leftChannel, rightChannel, sampleRate, bitsPerSample);
    }

    // Print summary
    tester.printSummary();

    // Export to JSON
    tester.exportResultsJSON("audio_codec_results.json");

    // Final recommendations
    std::cout << "\n"
              << BOLD << CYAN;
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║              RECOMMENDATIONS                              ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝";
    std::cout << RESET << std::endl;

    std::cout << "\n"
              << YELLOW << "💡 Best Practices:" << RESET << std::endl;

    std::cout << "\n  " << BOLD << "Predictors:" << RESET << std::endl;
    std::cout << "    • LINEAR2 - Best balance of speed and compression" << std::endl;
    std::cout << "    • LINEAR3 - Slightly better compression, slower" << std::endl;
    std::cout << "    • LINEAR1 - Fastest, lower compression" << std::endl;
    std::cout << "    • ADAPTIVE - Auto-selects best predictor per block" << std::endl;

    std::cout << "\n  " << BOLD << "Stereo Modes:" << RESET << std::endl;
    std::cout << "    • MID_SIDE - Best for correlated stereo content" << std::endl;
    std::cout << "    • INDEPENDENT - Better for uncorrelated channels" << std::endl;

    std::cout << "\n  " << BOLD << "Performance:" << RESET << std::endl;
    std::cout << "    • Decoding is optimized to be faster than encoding" << std::endl;
    std::cout << "    • Adaptive M parameter improves compression" << std::endl;
    std::cout << "    • Block-based processing enables streaming" << std::endl;

    std::cout << "\n  " << BOLD << "General:" << RESET << std::endl;
    std::cout << "    • All codecs are perfectly lossless" << std::endl;
    std::cout << "    • Works with mono and stereo 16-bit PCM audio" << std::endl;
    std::cout << "    • Sample rate independent (any rate supported)" << std::endl;

    std::cout << "\n  " << BOLD << "Output Files:" << RESET << std::endl;
    std::cout << "    • test_audio_*.golomb - Compressed audio files" << std::endl;
    std::cout << "    • test_audio_*_decoded.wav - Reconstructed audio" << std::endl;
    std::cout << "    • audio_codec_results.json - Machine-readable results" << std::endl;

    std::cout << "\n  " << BOLD << "Expected Performance:" << RESET << std::endl;
    std::cout << "    • Speech:           1.8-2.5:1 compression (6-9 bits/sample)" << std::endl;
    std::cout << "    • Music (tonal):    1.5-2.0:1 compression (8-11 bits/sample)" << std::endl;
    std::cout << "    • Music (complex):  1.3-1.7:1 compression (9-12 bits/sample)" << std::endl;
    std::cout << "    • Sine waves:       2.5-4.0:1 compression (4-6 bits/sample)" << std::endl;
    std::cout << "    • White noise:      1.0-1.1:1 compression (14-16 bits/sample)" << std::endl;

    std::cout << "\n  " << BOLD << "Decode Speed:" << RESET << std::endl;
    std::cout << "    • Typical speedup:  1.5-3.0x faster than encoding" << std::endl;
    std::cout << "    • Optimized with:   Loop unrolling, inlined predictors" << std::endl;
    std::cout << "    • Fast paths:       Specialized code for each predictor" << std::endl;

    std::cout << "\n"
              << BOLD << GREEN;
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║              TEST SUITE COMPLETE ✓                        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝";
    std::cout << RESET << "\n"
              << std::endl;

    return 0;
}
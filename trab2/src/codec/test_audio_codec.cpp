#include "../../includes/codec/audio_codec.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <map>

// ==================== Color Codes ====================
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

// ==================== WAV File Handling ====================

struct WavHeader {
    char riff[4];           // "RIFF"
    uint32_t fileSize;      // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmtSize;       // Format chunk size (16 for PCM)
    uint16_t audioFormat;   // 1 = PCM
    uint16_t numChannels;   // 1 = mono, 2 = stereo
    uint32_t sampleRate;    // 8000, 44100, etc.
    uint32_t byteRate;      // sampleRate * numChannels * bitsPerSample/8
    uint16_t blockAlign;    // numChannels * bitsPerSample/8
    uint16_t bitsPerSample; // 8, 16, 24, 32
    char data[4];           // "data"
    uint32_t dataSize;      // Size of audio data
};

class AudioFileHandler {
public:
    static bool readWAV(const std::string& filename, std::vector<int32_t>& audioData,
                       int& channels, int& sampleRate, int& bitsPerSample) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << RED << "✗ Cannot open file: " << filename << RESET << std::endl;
            return false;
        }

        WavHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));

        if (std::strncmp(header.riff, "RIFF", 4) != 0 ||
            std::strncmp(header.wave, "WAVE", 4) != 0) {
            std::cerr << RED << "✗ Not a valid WAV file" << RESET << std::endl;
            return false;
        }

        if (header.audioFormat != 1) {
            std::cerr << RED << "✗ Only PCM format is supported (format code: " 
                      << header.audioFormat << ")" << RESET << std::endl;
            return false;
        }

        channels = header.numChannels;
        sampleRate = header.sampleRate;
        bitsPerSample = header.bitsPerSample;

        size_t numSamples = header.dataSize / (bitsPerSample / 8);
        audioData.resize(numSamples);

        if (bitsPerSample == 8) {
            for (size_t i = 0; i < numSamples; i++) {
                uint8_t sample;
                file.read(reinterpret_cast<char*>(&sample), 1);
                audioData[i] = static_cast<int32_t>(sample) - 128;
            }
        } else if (bitsPerSample == 16) {
            std::vector<int16_t> buffer(numSamples);
            file.read(reinterpret_cast<char*>(buffer.data()), numSamples * 2);
            for (size_t i = 0; i < numSamples; i++) {
                audioData[i] = static_cast<int32_t>(buffer[i]);
            }
        } else if (bitsPerSample == 24) {
            for (size_t i = 0; i < numSamples; i++) {
                uint8_t bytes[3];
                file.read(reinterpret_cast<char*>(bytes), 3);
                int32_t sample = (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
                if (sample & 0x800000) sample |= 0xFF000000; // Sign extend
                audioData[i] = sample;
            }
        } else {
            std::cerr << RED << "✗ Unsupported bit depth: " << bitsPerSample 
                      << " bits" << RESET << std::endl;
            return false;
        }

        file.close();
        return true;
    }

    static bool writeWAV(const std::string& filename, const std::vector<int32_t>& audioData,
                        int channels, int sampleRate, int bitsPerSample) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << RED << "✗ Cannot create file: " << filename << RESET << std::endl;
            return false;
        }

        WavHeader header;
        std::memcpy(header.riff, "RIFF", 4);
        std::memcpy(header.wave, "WAVE", 4);
        std::memcpy(header.fmt, "fmt ", 4);
        std::memcpy(header.data, "data", 4);

        header.fmtSize = 16;
        header.audioFormat = 1;
        header.numChannels = channels;
        header.sampleRate = sampleRate;
        header.bitsPerSample = bitsPerSample;
        header.blockAlign = channels * bitsPerSample / 8;
        header.byteRate = sampleRate * header.blockAlign;
        header.dataSize = audioData.size() * bitsPerSample / 8;
        header.fileSize = 36 + header.dataSize;

        file.write(reinterpret_cast<char*>(&header), sizeof(WavHeader));

        if (bitsPerSample == 16) {
            for (int32_t sample : audioData) {
                int16_t val = static_cast<int16_t>(std::max(-32768, std::min(32767, sample)));
                file.write(reinterpret_cast<char*>(&val), 2);
            }
        }

        file.close();
        return true;
    }

    static void printAudioInfo(const std::string& filename, int channels, 
                              int sampleRate, int bitsPerSample, size_t numSamples) {
        double duration = static_cast<double>(numSamples) / channels / sampleRate;
        size_t fileSize = 44 + numSamples * bitsPerSample / 8;
        
        std::cout << CYAN << "📁 Audio File Information:" << RESET << std::endl;
        std::cout << "  ├─ File: " << filename << std::endl;
        std::cout << "  ├─ Channels: " << channels << " (" 
                  << (channels == 1 ? "Mono" : "Stereo") << ")" << std::endl;
        std::cout << "  ├─ Sample Rate: " << sampleRate << " Hz" << std::endl;
        std::cout << "  ├─ Bit Depth: " << bitsPerSample << " bits" << std::endl;
        std::cout << "  ├─ Samples: " << numSamples / channels << " per channel" << std::endl;
        std::cout << "  ├─ Duration: " << std::fixed << std::setprecision(2) 
                  << duration << " seconds" << std::endl;
        std::cout << "  └─ File Size: " << std::setprecision(2) 
                  << fileSize / 1024.0 << " KB" << std::endl;
    }
};

// ==================== Audio Signal Analysis ====================

class AudioAnalyzer {
public:
    struct AudioStatistics {
        double mean;
        double stdDev;
        double dynamicRange;
        double signalEnergy;
        double zeroCrossings;
        double autocorrelation;
        int32_t minValue;
        int32_t maxValue;
    };

    static AudioStatistics analyzeChannel(const std::vector<int32_t>& samples) {
        AudioStatistics stats = {0};
        
        if (samples.empty()) return stats;

        // Find min/max
        stats.minValue = *std::min_element(samples.begin(), samples.end());
        stats.maxValue = *std::max_element(samples.begin(), samples.end());
        
        // Calculate mean
        double sum = 0;
        for (int32_t s : samples) sum += s;
        stats.mean = sum / samples.size();
        
        // Calculate standard deviation and energy
        double sumSquaredDiff = 0;
        for (int32_t s : samples) {
            double diff = s - stats.mean;
            sumSquaredDiff += diff * diff;
            stats.signalEnergy += s * s;
        }
        stats.stdDev = std::sqrt(sumSquaredDiff / samples.size());
        stats.signalEnergy /= samples.size();
        
        // Dynamic range in dB
        if (stats.maxValue != 0) {
            stats.dynamicRange = 20 * std::log10(std::abs(stats.maxValue));
        }
        
        // Zero crossings (indicates frequency content)
        int crossings = 0;
        for (size_t i = 1; i < samples.size(); i++) {
            if ((samples[i-1] >= 0 && samples[i] < 0) || 
                (samples[i-1] < 0 && samples[i] >= 0)) {
                crossings++;
            }
        }
        stats.zeroCrossings = static_cast<double>(crossings) / samples.size();
        
        // First-order autocorrelation (measures temporal correlation)
        double autoCorr = 0;
        for (size_t i = 1; i < samples.size(); i++) {
            autoCorr += samples[i-1] * samples[i];
        }
        if (stats.signalEnergy > 0) {
            stats.autocorrelation = autoCorr / ((samples.size() - 1) * stats.signalEnergy);
        }
        
        return stats;
    }

    static void printStatistics(const std::string& label, const AudioStatistics& stats) {
        std::cout << YELLOW << "  " << label << " Channel Statistics:" << RESET << std::endl;
        std::cout << "    ├─ Range: [" << stats.minValue << ", " << stats.maxValue << "]" << std::endl;
        std::cout << "    ├─ Mean: " << std::fixed << std::setprecision(2) << stats.mean << std::endl;
        std::cout << "    ├─ Std Dev: " << stats.stdDev << std::endl;
        std::cout << "    ├─ Dynamic Range: " << stats.dynamicRange << " dB" << std::endl;
        std::cout << "    ├─ Zero Crossings: " << stats.zeroCrossings << std::endl;
        std::cout << "    └─ Autocorrelation: " << std::setprecision(4) << stats.autocorrelation << std::endl;
    }
};

// ==================== Synthetic Audio Generation ====================

class AudioGenerator {
public:
    static std::vector<int32_t> createTestAudio(int channels, int sampleRate, 
                                                double duration, int bitsPerSample,
                                                const std::string& type = "sine") {
        size_t numSamples = static_cast<size_t>(sampleRate * duration) * channels;
        std::vector<int32_t> audio(numSamples);
        int32_t maxAmplitude = (1 << (bitsPerSample - 1)) - 1;

        if (type == "sine") {
            return generateSineWave(audio, channels, sampleRate, maxAmplitude);
        } else if (type == "chord") {
            return generateChord(audio, channels, sampleRate, maxAmplitude);
        } else if (type == "sweep") {
            return generateFrequencySweep(audio, channels, sampleRate, maxAmplitude, duration);
        } else if (type == "noise") {
            return generateNoise(audio, channels, maxAmplitude);
        } else if (type == "speech") {
            return generateSpeechLike(audio, channels, sampleRate, maxAmplitude);
        }
        
        return audio;
    }

private:
    static std::vector<int32_t> generateSineWave(std::vector<int32_t>& audio, 
                                                 int channels, int sampleRate, 
                                                 int32_t maxAmplitude) {
        double freq1 = 440.0;  // A4 note
        double freq2 = 554.37; // C#5 note
        
        for (size_t i = 0; i < audio.size() / channels; i++) {
            double t = static_cast<double>(i) / sampleRate;
            
            if (channels == 1) {
                audio[i] = static_cast<int32_t>(
                    maxAmplitude * 0.7 * std::sin(2 * M_PI * freq1 * t)
                );
            } else {
                audio[i * 2] = static_cast<int32_t>(
                    maxAmplitude * 0.7 * std::sin(2 * M_PI * freq1 * t)
                );
                audio[i * 2 + 1] = static_cast<int32_t>(
                    maxAmplitude * 0.5 * std::sin(2 * M_PI * freq2 * t)
                );
            }
        }
        return audio;
    }

    static std::vector<int32_t> generateChord(std::vector<int32_t>& audio, 
                                              int channels, int sampleRate, 
                                              int32_t maxAmplitude) {
        // C major chord: C4, E4, G4
        double freqs[] = {261.63, 329.63, 392.00};
        
        for (size_t i = 0; i < audio.size() / channels; i++) {
            double t = static_cast<double>(i) / sampleRate;
            double sample = 0;
            
            for (double freq : freqs) {
                sample += std::sin(2 * M_PI * freq * t) / 3.0;
            }
            
            if (channels == 1) {
                audio[i] = static_cast<int32_t>(maxAmplitude * 0.7 * sample);
            } else {
                audio[i * 2] = static_cast<int32_t>(maxAmplitude * 0.7 * sample);
                audio[i * 2 + 1] = static_cast<int32_t>(maxAmplitude * 0.6 * sample);
            }
        }
        return audio;
    }

    static std::vector<int32_t> generateFrequencySweep(std::vector<int32_t>& audio,
                                                       int channels, int sampleRate,
                                                       int32_t maxAmplitude, double duration) {
        double startFreq = 100.0;
        double endFreq = 2000.0;
        
        for (size_t i = 0; i < audio.size() / channels; i++) {
            double t = static_cast<double>(i) / sampleRate;
            double progress = t / duration;
            double freq = startFreq + (endFreq - startFreq) * progress;
            double sample = std::sin(2 * M_PI * freq * t);
            
            if (channels == 1) {
                audio[i] = static_cast<int32_t>(maxAmplitude * 0.7 * sample);
            } else {
                audio[i * 2] = static_cast<int32_t>(maxAmplitude * 0.7 * sample);
                audio[i * 2 + 1] = static_cast<int32_t>(maxAmplitude * 0.6 * sample);
            }
        }
        return audio;
    }

    static std::vector<int32_t> generateNoise(std::vector<int32_t>& audio,
                                              int channels, int32_t maxAmplitude) {
        for (size_t i = 0; i < audio.size(); i++) {
            audio[i] = (rand() % (2 * maxAmplitude)) - maxAmplitude;
            audio[i] = audio[i] / 4; // Reduce amplitude
        }
        return audio;
    }

    static std::vector<int32_t> generateSpeechLike(std::vector<int32_t>& audio,
                                                   int channels, int sampleRate,
                                                   int32_t maxAmplitude) {
        // Speech-like signal with formants
        double formants[] = {700, 1220, 2600}; // Typical vowel formants
        
        for (size_t i = 0; i < audio.size() / channels; i++) {
            double t = static_cast<double>(i) / sampleRate;
            double sample = 0;
            
            // Pitch variation (100-200 Hz)
            double pitch = 150 + 50 * std::sin(2 * M_PI * 5 * t);
            sample += std::sin(2 * M_PI * pitch * t);
            
            // Add formants
            for (double formant : formants) {
                sample += 0.3 * std::sin(2 * M_PI * formant * t);
            }
            
            // Amplitude modulation (speech envelope)
            double envelope = 0.5 + 0.5 * std::sin(2 * M_PI * 3 * t);
            sample *= envelope;
            
            if (channels == 1) {
                audio[i] = static_cast<int32_t>(maxAmplitude * 0.4 * sample);
            } else {
                audio[i * 2] = static_cast<int32_t>(maxAmplitude * 0.4 * sample);
                audio[i * 2 + 1] = static_cast<int32_t>(maxAmplitude * 0.35 * sample);
            }
        }
        return audio;
    }
};

// ==================== Testing Framework ====================

class AudioCodecTester {
private:
    struct TestResult {
        std::string testName;
        AudioCodec::PredictorType predictor;
        AudioCodec::StereoMode stereoMode;
        double compressionRatio;
        double bitsPerSample;
        double spaceSavings;
        int optimalM;
        long long encodeTimeMs;
        long long decodeTimeMs;
        bool reconstructionPerfect;
        size_t originalSize;
        size_t compressedSize;
    };

    std::vector<TestResult> results;

public:
    void runTest(const std::string& testName,
                AudioCodec::PredictorType predictor,
                AudioCodec::StereoMode stereoMode,
                const std::vector<int32_t>& audioData,
                int channels, int sampleRate, int bitsPerSample) {
        
        printTestHeader(testName);

        AudioCodec codec(predictor, stereoMode, 0, true);

        std::string compressedFile = "test_aud_" + testName + ".golomb";
        std::string decodedFile = "test_aud_" + testName + "_decoded.wav";

        // Encode
        auto startEncode = std::chrono::high_resolution_clock::now();
        if (!codec.encode(audioData, channels, sampleRate, bitsPerSample, compressedFile)) {
            std::cerr << RED << "✗ Encoding failed!" << RESET << std::endl;
            return;
        }
        auto endEncode = std::chrono::high_resolution_clock::now();
        auto encodeTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            endEncode - startEncode).count();

        auto stats = codec.getLastStats();
        
        printCompressionResults(stats, encodeTime);

        // Decode
        auto startDecode = std::chrono::high_resolution_clock::now();
        std::vector<int32_t> decoded;
        int decChannels, decSampleRate, decBitsPerSample;

        if (!codec.decode(compressedFile, decoded, decChannels, decSampleRate, decBitsPerSample)) {
            std::cerr << RED << "✗ Decoding failed!" << RESET << std::endl;
            return;
        }
        auto endDecode = std::chrono::high_resolution_clock::now();
        auto decodeTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            endDecode - startDecode).count();

        std::cout << "\n⚙️  Decoding time: " << decodeTime << " ms" << std::endl;

        // Verify
        bool identical = verifyReconstruction(audioData, decoded);
        
        if (identical) {
            std::cout << GREEN << "\n✓ Perfect reconstruction verified!" << RESET << std::endl;
        } else {
            std::cerr << RED << "\n✗ Decoded audio differs from original!" << RESET << std::endl;
        }

        AudioFileHandler::writeWAV(decodedFile, decoded, decChannels, decSampleRate, decBitsPerSample);

        // Store results
        TestResult result;
        result.testName = testName;
        result.predictor = predictor;
        result.stereoMode = stereoMode;
        result.compressionRatio = stats.compressionRatio;
        result.bitsPerSample = stats.bitsPerSample;
        result.spaceSavings = 100.0 * (1.0 - 1.0/stats.compressionRatio);
        result.optimalM = stats.optimalM;
        result.encodeTimeMs = encodeTime;
        result.decodeTimeMs = decodeTime;
        result.reconstructionPerfect = identical;
        result.originalSize = stats.originalSize;
        result.compressedSize = stats.compressedSize;
        results.push_back(result);
    }

    void printSummary() {
        if (results.empty()) return;

        std::cout << "\n" << BOLD << CYAN;
        std::cout << "╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║              COMPRESSION SUMMARY                           ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝";
        std::cout << RESET << std::endl;

        // Find best configuration
        auto best = std::max_element(results.begin(), results.end(),
            [](const TestResult& a, const TestResult& b) {
                return a.compressionRatio < b.compressionRatio;
            });

        std::cout << "\n" << GREEN << "🏆 Best Configuration:" << RESET << std::endl;
        std::cout << "  Test: " << BOLD << best->testName << RESET << std::endl;
        std::cout << "  Compression Ratio: " << BOLD << std::fixed << std::setprecision(2) 
                  << best->compressionRatio << ":1" << RESET << std::endl;
        std::cout << "  Bits per Sample: " << std::setprecision(3) 
                  << best->bitsPerSample << std::endl;
        std::cout << "  Space Savings: " << std::setprecision(1) 
                  << best->spaceSavings << "%" << std::endl;

        // Comparison table
        std::cout << "\n" << YELLOW << "📊 All Results:" << RESET << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        std::cout << std::left << std::setw(25) << "Test Name" 
                  << std::setw(12) << "Ratio" 
                  << std::setw(12) << "Bits/Samp"
                  << std::setw(12) << "Enc (ms)"
                  << std::setw(12) << "Dec (ms)"
                  << "Status" << std::endl;
        std::cout << std::string(80, '-') << std::endl;

        for (const auto& r : results) {
            std::cout << std::left << std::setw(25) << r.testName
                      << std::setw(12) << std::fixed << std::setprecision(2) << r.compressionRatio
                      << std::setw(12) << std::setprecision(3) << r.bitsPerSample
                      << std::setw(12) << r.encodeTimeMs
                      << std::setw(12) << r.decodeTimeMs;
            
            if (r.reconstructionPerfect) {
                std::cout << GREEN << "✓ OK" << RESET;
            } else {
                std::cout << RED << "✗ FAIL" << RESET;
            }
            std::cout << std::endl;
        }
        std::cout << std::string(80, '-') << std::endl;

        // Statistics
        double avgRatio = 0, avgBits = 0;
        for (const auto& r : results) {
            avgRatio += r.compressionRatio;
            avgBits += r.bitsPerSample;
        }
        avgRatio /= results.size();
        avgBits /= results.size();

        std::cout << "\n" << CYAN << "📈 Average Statistics:" << RESET << std::endl;
        std::cout << "  Compression Ratio: " << std::fixed << std::setprecision(2) 
                  << avgRatio << ":1" << std::endl;
        std::cout << "  Bits per Sample: " << std::setprecision(3) << avgBits << std::endl;
    }

private:
    void printTestHeader(const std::string& testName) {
        std::cout << "\n" << std::string(70, '─') << std::endl;
        std::cout << BOLD << BLUE << "🎵 Audio Test: " << testName << RESET << std::endl;
        std::cout << std::string(70, '─') << std::endl;
    }

    void printCompressionResults(const AudioCodec::CompressionStats& stats, long long encodeTime) {
        std::cout << "\n" << MAGENTA << "📊 Compression Results:" << RESET << std::endl;
        std::cout << "  ├─ Compression ratio:   " << std::fixed << std::setprecision(2) 
                  << stats.compressionRatio << ":1" << std::endl;
        std::cout << "  ├─ Bits per sample:     " << std::setprecision(3) 
                  << stats.bitsPerSample << " (original: " << stats.bitsPerSampleOriginal << ")" << std::endl;
        std::cout << "  ├─ Space savings:       " << std::setprecision(1) 
                  << (100.0 * (1.0 - 1.0/stats.compressionRatio)) << "%" << std::endl;
        std::cout << "  ├─ Original size:       " << stats.originalSize << " bytes" << std::endl;
        std::cout << "  ├─ Compressed size:     " << stats.compressedSize << " bytes" << std::endl;
        std::cout << "  ├─ Optimal m:           " << stats.optimalM << std::endl;
        
        if (stats.channels == 2) {
            std::cout << "  ├─ Stereo mode:         ";
            if (stats.bestStereoMode == AudioCodec::StereoMode::MID_SIDE) {
                std::cout << "Mid-Side (exploits correlation)";
            } else if (stats.bestStereoMode == AudioCodec::StereoMode::INDEPENDENT) {
                std::cout << "Independent (separate channels)";
            } else {
                std::cout << "Adaptive";
            }
            std::cout << std::endl;
        }
        
        std::cout << "  └─ Encoding time:       " << encodeTime << " ms" << std::endl;
    }

    bool verifyReconstruction(const std::vector<int32_t>& original, 
                             const std::vector<int32_t>& decoded) {
        if (original.size() != decoded.size()) {
            std::cerr << YELLOW << "  Size mismatch: " << original.size() 
                      << " vs " << decoded.size() << RESET << std::endl;
            return false;
        }

        for (size_t i = 0; i < original.size(); i++) {
            if (original[i] != decoded[i]) {
                std::cerr << YELLOW << "  Difference at sample " << i << ": "
                          << original[i] << " vs " << decoded[i] << RESET << std::endl;
                return false;
            }
        }
        return true;
    }
};

// ==================== Main Program ====================

void printBanner() {
    std::cout << BOLD << CYAN;
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║           Audio Codec Test Suite - Golomb Coding          ║\n";
    std::cout << "║                   Lossless Audio Compression               ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << RESET << std::endl;
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [audio.wav] [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  No arguments     - Run with synthetic test audio (2s stereo)\n";
    std::cout << "  audio.wav        - Test with your WAV file\n";
    std::cout << "  -h, --help       - Show this help message\n";
    std::cout << "  -v, --verbose    - Show detailed analysis\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << "\n";
    std::cout << "  " << programName << " mysong.wav\n";
    std::cout << "  " << programName << " /path/to/audio.wav\n";
}

int main(int argc, char* argv[]) {
    printBanner();

    // Parse arguments
    bool verbose = false;
    std::string inputFile = "";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg[0] != '-') {
            inputFile = arg;
        }
    }

    // Load or generate audio
    std::vector<int32_t> audioData;
    int channels, sampleRate, bitsPerSample;
    std::string audioSource;

    if (!inputFile.empty()) {
        std::cout << CYAN << "📂 Loading audio from: " << inputFile << RESET << std::endl;
        if (!AudioFileHandler::readWAV(inputFile, audioData, channels, sampleRate, bitsPerSample)) {
            std::cerr << RED << "Failed to load audio file" << RESET << std::endl;
            std::cout << YELLOW << "\n💡 Creating synthetic audio instead..." << RESET << std::endl;
            inputFile = "";
        } else {
            AudioFileHandler::printAudioInfo(inputFile, channels, sampleRate, bitsPerSample, audioData.size());
            audioSource = inputFile;
        }
    }

    if (inputFile.empty()) {
        std::cout << YELLOW << "\n⚠️  No audio file provided. Creating synthetic test audio..." << RESET << std::endl;
        channels = 2;
        sampleRate = 44100;
        bitsPerSample = 16;
        double duration = 2.0;

        std::cout << "  ├─ Type: Stereo sine wave (A4 + C#5)" << std::endl;
        std::cout << "  ├─ Channels: " << channels << std::endl;
        std::cout << "  ├─ Sample Rate: " << sampleRate << " Hz" << std::endl;
        std::cout << "  ├─ Bit Depth: " << bitsPerSample << " bits" << std::endl;
        std::cout << "  └─ Duration: " << duration << " seconds" << std::endl;

        audioData = AudioGenerator::createTestAudio(channels, sampleRate, duration, bitsPerSample, "sine");
        
        std::string synthFile = "test_audio_original.wav";
        AudioFileHandler::writeWAV(synthFile, audioData, channels, sampleRate, bitsPerSample);
        std::cout << GREEN << "\n✓ Synthetic audio saved as: " << synthFile << RESET << std::endl;
        audioSource = synthFile;
    }

    // Analyze audio if verbose
    if (verbose) {
        std::cout << "\n" << BOLD << MAGENTA << "🔍 Audio Signal Analysis:" << RESET << std::endl;
        
        if (channels == 1) {
            auto stats = AudioAnalyzer::analyzeChannel(audioData);
            AudioAnalyzer::printStatistics("Mono", stats);
        } else {
            // Split stereo channels
            std::vector<int32_t> left, right;
            for (size_t i = 0; i < audioData.size(); i += 2) {
                left.push_back(audioData[i]);
                right.push_back(audioData[i + 1]);
            }
            
            auto statsL = AudioAnalyzer::analyzeChannel(left);
            auto statsR = AudioAnalyzer::analyzeChannel(right);
            
            AudioAnalyzer::printStatistics("Left", statsL);
            AudioAnalyzer::printStatistics("Right", statsR);
            
            // Channel correlation
            double correlation = 0;
            for (size_t i = 0; i < left.size(); i++) {
                correlation += left[i] * right[i];
            }
            correlation /= (statsL.signalEnergy * left.size());
            
            std::cout << YELLOW << "\n  Stereo Correlation:" << RESET << std::endl;
            std::cout << "    └─ L-R Correlation: " << std::fixed << std::setprecision(4) 
                      << correlation << std::endl;
            
            if (correlation > 0.8) {
                std::cout << GREEN << "       (High correlation - Mid-Side coding recommended)" 
                          << RESET << std::endl;
            } else if (correlation > 0.5) {
                std::cout << YELLOW << "       (Moderate correlation - Both modes viable)" 
                          << RESET << std::endl;
            } else {
                std::cout << CYAN << "       (Low correlation - Independent coding recommended)" 
                          << RESET << std::endl;
            }
        }
    }

    // Initialize tester
    AudioCodecTester tester;

    // Run tests based on channel configuration
    std::cout << "\n" << BOLD << GREEN;
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              STARTING COMPRESSION TESTS                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝";
    std::cout << RESET << std::endl;

    if (channels == 1) {
        std::cout << "\n" << BOLD << YELLOW << "--- Testing Mono Audio ---" << RESET << std::endl;

        tester.runTest("Mono_Linear1", 
                      AudioCodec::PredictorType::LINEAR_1,
                      AudioCodec::StereoMode::INDEPENDENT,
                      audioData, channels, sampleRate, bitsPerSample);

        tester.runTest("Mono_Linear2", 
                      AudioCodec::PredictorType::LINEAR_2,
                      AudioCodec::StereoMode::INDEPENDENT,
                      audioData, channels, sampleRate, bitsPerSample);

        tester.runTest("Mono_Linear3", 
                      AudioCodec::PredictorType::LINEAR_3,
                      AudioCodec::StereoMode::INDEPENDENT,
                      audioData, channels, sampleRate, bitsPerSample);

        tester.runTest("Mono_Adaptive", 
                      AudioCodec::PredictorType::ADAPTIVE,
                      AudioCodec::StereoMode::INDEPENDENT,
                      audioData, channels, sampleRate, bitsPerSample);

    } else {
        std::cout << "\n" << BOLD << YELLOW << "--- Testing Stereo Audio ---" << RESET << std::endl;

        tester.runTest("Stereo_Independent_L1", 
                      AudioCodec::PredictorType::LINEAR_1,
                      AudioCodec::StereoMode::INDEPENDENT,
                      audioData, channels, sampleRate, bitsPerSample);

        tester.runTest("Stereo_Independent_L2", 
                      AudioCodec::PredictorType::LINEAR_2,
                      AudioCodec::StereoMode::INDEPENDENT,
                      audioData, channels, sampleRate, bitsPerSample);

        tester.runTest("Stereo_Independent_L3", 
                      AudioCodec::PredictorType::LINEAR_3,
                      AudioCodec::StereoMode::INDEPENDENT,
                      audioData, channels, sampleRate, bitsPerSample);

        tester.runTest("Stereo_MidSide_L1", 
                      AudioCodec::PredictorType::LINEAR_1,
                      AudioCodec::StereoMode::MID_SIDE,
                      audioData, channels, sampleRate, bitsPerSample);

        tester.runTest("Stereo_MidSide_L2", 
                      AudioCodec::PredictorType::LINEAR_2,
                      AudioCodec::StereoMode::MID_SIDE,
                      audioData, channels, sampleRate, bitsPerSample);

        tester.runTest("Stereo_MidSide_L3", 
                      AudioCodec::PredictorType::LINEAR_3,
                      AudioCodec::StereoMode::MID_SIDE,
                      audioData, channels, sampleRate, bitsPerSample);

        tester.runTest("Stereo_Adaptive", 
                      AudioCodec::PredictorType::ADAPTIVE,
                      AudioCodec::StereoMode::ADAPTIVE,
                      audioData, channels, sampleRate, bitsPerSample);
    }

    // Print summary
    tester.printSummary();

    // Final recommendations
    std::cout << "\n" << BOLD << CYAN;
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              RECOMMENDATIONS                               ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝";
    std::cout << RESET << std::endl;

    std::cout << "\n" << YELLOW << "💡 Best Practices:" << RESET << std::endl;

    if (channels == 1) {
        std::cout << "\n  " << BOLD << "Mono Audio:" << RESET << std::endl;
        std::cout << "    • LINEAR_2 or LINEAR_3 for smooth signals (music, tones)" << std::endl;
        std::cout << "    • ADAPTIVE for mixed content (speech, effects)" << std::endl;
        std::cout << "    • LINEAR_1 for very simple/fast compression" << std::endl;
    } else {
        std::cout << "\n  " << BOLD << "Stereo Audio:" << RESET << std::endl;
        std::cout << "    • MID_SIDE mode for correlated channels (music, ambiance)" << std::endl;
        std::cout << "    • INDEPENDENT mode for uncorrelated channels (dual mono)" << std::endl;
        std::cout << "    • ADAPTIVE mode for unknown/mixed content" << std::endl;
        std::cout << "    • LINEAR_2 predictor offers best balance" << std::endl;
    }

    std::cout << "\n  " << BOLD << "General:" << RESET << std::endl;
    std::cout << "    • All codecs are perfectly lossless" << std::endl;
    std::cout << "    • Adaptive m parameter optimizes per block" << std::endl;
    std::cout << "    • Higher predictors (LINEAR_3) better for smooth signals" << std::endl;
    std::cout << "    • Compression varies with signal characteristics" << std::endl;

    std::cout << "\n  " << BOLD << "Output Files:" << RESET << std::endl;
    std::cout << "    • test_aud_*.golomb - Compressed audio files" << std::endl;
    std::cout << "    • test_aud_*_decoded.wav - Reconstructed audio files" << std::endl;
    std::cout << "    • All files saved in current directory" << std::endl;

    // Performance insights
    std::cout << "\n  " << BOLD << "Expected Performance:" << RESET << std::endl;
    std::cout << "    • Music (stereo):  1.2-1.8:1 compression (9-13 bits/sample)" << std::endl;
    std::cout << "    • Speech (mono):   1.3-2.0:1 compression (8-12 bits/sample)" << std::endl;
    std::cout << "    • Tones (pure):    2.0-4.0:1 compression (4-8 bits/sample)" << std::endl;
    std::cout << "    • Noise (random):  1.0-1.2:1 compression (13-16 bits/sample)" << std::endl;

    std::cout << "\n" << BOLD << GREEN;
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              TEST SUITE COMPLETE ✓                         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝";
    std::cout << RESET << "\n" << std::endl;

    return 0;
}
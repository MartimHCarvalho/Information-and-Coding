#ifndef AUDIO_CODEC_HPP
#define AUDIO_CODEC_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "../golomb.hpp"
#include "../bitstream.hpp"

class AudioCodec {
public:
    enum class PredictorType {
        LINEAR_1,        // x[n-1]
        LINEAR_2,        // 2*x[n-1] - x[n-2]
        LINEAR_3,        // 3*x[n-1] - 3*x[n-2] + x[n-3]
        ADAPTIVE         // Selects best predictor per block
    };

    enum class StereoMode {
        INDEPENDENT,     // Encode left and right independently
        MID_SIDE,        // Mid-side stereo coding
        ADAPTIVE         // Choose best per block
    };

    struct CompressionStats {
        double compressionRatio;
        double bitsPerSample;
        int optimalM;
        PredictorType bestPredictor;
        StereoMode bestStereoMode;
        int channels;
        int sampleRate;
        int bitsPerSampleOriginal;
        size_t originalSize;
        size_t compressedSize;
    };

private:
    PredictorType predictor;
    StereoMode stereoMode;
    int m;  // Golomb parameter (0 for auto)
    bool adaptive;
    CompressionStats lastStats;
    
    // Block size for adaptive m
    static const int BLOCK_SIZE = 1024;
    static const int MIN_M = 4;
    static const int MAX_M = 256;

    // Predictor functions
    int32_t predictLinear1(const std::vector<int32_t>& samples, size_t pos);
    int32_t predictLinear2(const std::vector<int32_t>& samples, size_t pos);
    int32_t predictLinear3(const std::vector<int32_t>& samples, size_t pos);
    int32_t predictAdaptive(const std::vector<int32_t>& samples, size_t pos, 
                            PredictorType& usedPredictor);

    // Stereo encoding helpers
    void midSideTransform(const std::vector<int32_t>& left, 
                         const std::vector<int32_t>& right,
                         std::vector<int32_t>& mid, 
                         std::vector<int32_t>& side);
    
    void inverseMidSideTransform(const std::vector<int32_t>& mid, 
                                const std::vector<int32_t>& side,
                                std::vector<int32_t>& left, 
                                std::vector<int32_t>& right);

    // Optimal m estimation
    int estimateOptimalM(const std::vector<int32_t>& residuals, 
                        size_t start, size_t count);

    // Encode/decode single channel
    bool encodeChannel(const std::vector<int32_t>& samples, 
                      BitStream& bs, int& optimalM);
    
    bool decodeChannel(std::vector<int32_t>& samples, 
                      size_t numSamples, BitStream& bs);

public:
    AudioCodec(PredictorType predictor = PredictorType::ADAPTIVE,
              StereoMode stereoMode = StereoMode::ADAPTIVE,
              int m = 0, 
              bool adaptive = true);

    ~AudioCodec();

    // Main encode/decode functions
    bool encode(const std::vector<int32_t>& audioData, 
               int channels, 
               int sampleRate,
               int bitsPerSample,
               const std::string& outputFile);

    bool decode(const std::string& inputFile,
               std::vector<int32_t>& audioData,
               int& channels,
               int& sampleRate,
               int& bitsPerSample);

    // Get compression statistics
    CompressionStats getLastStats() const { return lastStats; }

    // Setters
    void setPredictor(PredictorType p) { predictor = p; }
    void setStereoMode(StereoMode mode) { stereoMode = mode; }
    void setM(int newM) { m = newM; }
    void setAdaptive(bool adapt) { adaptive = adapt; }
};

#endif // AUDIO_CODEC_HPP
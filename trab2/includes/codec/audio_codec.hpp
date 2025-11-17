#ifndef AUDIO_CODEC_HPP
#define AUDIO_CODEC_HPP

#include <vector>
#include <string>
#include <cstdint>

// Forward declaration
class BitStream;

class AudioCodec
{
public:
    // Predictor types
    enum class PredictorType
    {
        NONE,    // No prediction (encode raw samples)
        LINEAR1, // First-order: x[n-1]
        LINEAR2, // Second-order: 2*x[n-1] - x[n-2]
        LINEAR3, // Third-order: 3*x[n-1] - 3*x[n-2] + x[n-3]
        ADAPTIVE // Adaptively choose best predictor per block
    };

    // Channel coding modes for stereo
    enum class ChannelMode
    {
        INDEPENDENT, // Encode left and right independently
        MID_SIDE,    // Mid-side stereo coding: M=(L+R)/2, S=L-R
        LEFT_SIDE    // Left-side coding: L=L, S=L-R
    };

    // Compression statistics
    struct CompressionStats
    {
        size_t originalSize;     // Original size in bytes
        size_t compressedSize;   // Compressed size in bytes
        double compressionRatio; // Original / Compressed
        double bitsPerSample;    // Bits per sample after compression
        int optimalM;            // Optimal M parameter used
    };

    // Constructors
    AudioCodec(PredictorType predictor = PredictorType::LINEAR2,
               ChannelMode channelMode = ChannelMode::INDEPENDENT,
               int fixedM = 0, bool adaptiveM = true);
    AudioCodec();

    // Set block size for adaptive M calculation
    void setBlockSize(int blockSize);

    // Encode mono audio
    bool encodeMono(const std::vector<int16_t> &samples,
                    uint32_t sampleRate, uint16_t bitsPerSample,
                    const std::string &outputFile);

    // Encode stereo audio
    bool encodeStereo(const std::vector<int16_t> &left,
                      const std::vector<int16_t> &right,
                      uint32_t sampleRate, uint16_t bitsPerSample,
                      const std::string &outputFile);

    // Decode audio (auto-detects mono/stereo)
    bool decode(const std::string &inputFile,
                std::vector<int16_t> &left, std::vector<int16_t> &right,
                uint32_t &sampleRate, uint16_t &numChannels,
                uint16_t &bitsPerSample);

    // Get statistics from last encoding operation
    CompressionStats getLastStats() const;

private:
    PredictorType predictor_;
    ChannelMode channelMode_;
    int fixedM_;
    bool adaptiveM_;
    size_t blockSize_;
    CompressionStats lastStats_;

    // Prediction functions
    int16_t predictSample(const std::vector<int16_t> &history,
                          size_t pos, PredictorType type) const;

    // M parameter calculation
    int calculateOptimalM(const std::vector<int16_t> &residuals) const;

    // Channel encoding/decoding
    void applyMidSideEncoding(const std::vector<int16_t> &left,
                              const std::vector<int16_t> &right,
                              std::vector<int16_t> &mid,
                              std::vector<int16_t> &side) const;

    void applyMidSideDecoding(const std::vector<int16_t> &mid,
                              const std::vector<int16_t> &side,
                              std::vector<int16_t> &left,
                              std::vector<int16_t> &right) const;

    void applyLeftSideEncoding(const std::vector<int16_t> &left,
                               const std::vector<int16_t> &right,
                               std::vector<int16_t> &leftOut,
                               std::vector<int16_t> &side) const;

    void applyLeftSideDecoding(const std::vector<int16_t> &leftIn,
                               const std::vector<int16_t> &side,
                               std::vector<int16_t> &left,
                               std::vector<int16_t> &right) const;

    // Helper encoding/decoding functions
    void encodeChannel(BitStream &bs, const std::vector<int16_t> &samples);
    bool decodeChannel(BitStream &bs, std::vector<int16_t> &samples,
                       size_t totalSamples, size_t blockSize,
                       PredictorType predictor);

    bool decodeMono(BitStream &bs, std::vector<int16_t> &left,
                    std::vector<int16_t> &right, uint32_t &sampleRate,
                    uint16_t &numChannels, uint16_t &bitsPerSample);

    bool decodeStereo(BitStream &bs, std::vector<int16_t> &left,
                      std::vector<int16_t> &right, uint32_t &sampleRate,
                      uint16_t &numChannels, uint16_t &bitsPerSample);
};

#endif // AUDIO_CODEC_HPP
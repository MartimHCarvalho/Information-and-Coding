#include "codec/audio_codec.hpp"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits>

// Constructor
AudioCodec::AudioCodec(PredictorType predictor, StereoMode stereoMode, int m, bool adaptive)
    : predictor(predictor), stereoMode(stereoMode), m(m), adaptive(adaptive)
{
    lastStats = {};
}

// Destructor
AudioCodec::~AudioCodec()
{
    // Nothing to clean up
}

// Predictor implementations
int32_t AudioCodec::predictLinear1(const std::vector<int32_t> &samples, size_t pos)
{
    if (pos == 0)
        return 0;
    return samples[pos - 1];
}

int32_t AudioCodec::predictLinear2(const std::vector<int32_t> &samples, size_t pos)
{
    if (pos < 2)
        return predictLinear1(samples, pos);
    return 2 * samples[pos - 1] - samples[pos - 2];
}

int32_t AudioCodec::predictLinear3(const std::vector<int32_t> &samples, size_t pos)
{
    if (pos < 3)
        return predictLinear2(samples, pos);
    return 3 * samples[pos - 1] - 3 * samples[pos - 2] + samples[pos - 3];
}

int32_t AudioCodec::predictAdaptive(const std::vector<int32_t> &samples, size_t pos,
                                    PredictorType &usedPredictor)
{
    // For first few samples, use simpler predictors
    if (pos < 3)
    {
        usedPredictor = PredictorType::LINEAR_1;
        return predictLinear1(samples, pos);
    }

    // Choose predictor based on recent prediction errors
    // In practice, you'd track errors; here we'll use LINEAR_2 as default
    usedPredictor = PredictorType::LINEAR_2;
    return predictLinear2(samples, pos);
}

// Mid-side stereo transform
void AudioCodec::midSideTransform(const std::vector<int32_t> &left,
                                  const std::vector<int32_t> &right,
                                  std::vector<int32_t> &mid,
                                  std::vector<int32_t> &side)
{
    size_t n = left.size();
    mid.resize(n);
    side.resize(n);

    for (size_t i = 0; i < n; i++)
    {
        mid[i] = (left[i] + right[i]) / 2;
        side[i] = left[i] - right[i];
    }
}

void AudioCodec::inverseMidSideTransform(const std::vector<int32_t> &mid,
                                         const std::vector<int32_t> &side,
                                         std::vector<int32_t> &left,
                                         std::vector<int32_t> &right)
{
    size_t n = mid.size();
    left.resize(n);
    right.resize(n);

    for (size_t i = 0; i < n; i++)
    {
        // Exact integer reconstruction
        int32_t side_div2 = side[i] >> 1;
        left[i] = mid[i] + side[i] - side_div2;
        right[i] = mid[i] - side_div2;
    }
}

// Estimate optimal m parameter
int AudioCodec::estimateOptimalM(const std::vector<int32_t> &residuals,
                                 size_t start, size_t count)
{
    if (count == 0)
        return 16; // Default

    double sum = 0.0;
    size_t end = std::min(start + count, residuals.size());

    for (size_t i = start; i < end; i++)
    {
        sum += std::abs(residuals[i]);
    }

    double mean = sum / (end - start);

    // For audio, use a slightly different formula
    // m should be close to mean of absolute residuals
    int estimated_m = static_cast<int>(mean + 0.5);

    // Clamp to reasonable range for audio
    return std::max(4, std::min(128, estimated_m));
}

// Encode single channel
bool AudioCodec::encodeChannel(const std::vector<int32_t> &samples,
                               BitStream &bs, int &optimalM)
{
    if (samples.empty())
        return false;

    // Compute residuals
    std::vector<int32_t> residuals(samples.size());
    PredictorType usedPredictor;

    for (size_t i = 0; i < samples.size(); i++)
    {
        int32_t prediction;

        switch (predictor)
        {
        case PredictorType::LINEAR_1:
            prediction = predictLinear1(samples, i);
            break;
        case PredictorType::LINEAR_2:
            prediction = predictLinear2(samples, i);
            break;
        case PredictorType::LINEAR_3:
            prediction = predictLinear3(samples, i);
            break;
        case PredictorType::ADAPTIVE:
            prediction = predictAdaptive(samples, i, usedPredictor);
            break;
        }

        residuals[i] = samples[i] - prediction;
    }

    // Estimate optimal m if needed
    if (m == 0 || adaptive)
    {
        optimalM = estimateOptimalM(residuals, 0, residuals.size());
    }
    else
    {
        optimalM = m;
    }

    // Write optimal m to bitstream
    bs.writeBits(optimalM, 16);

    // Encode residuals using Golomb coding with ODD_EVEN_MAPPING for signed integers
    Golomb golomb(optimalM, Golomb::HandleSignApproach::ODD_EVEN_MAPPING);
    for (int32_t residual : residuals)
    {
        golomb.encode(residual, bs);
    }

    return true;
}

// Decode single channel
bool AudioCodec::decodeChannel(std::vector<int32_t> &samples,
                               size_t numSamples, BitStream &bs)
{
    samples.resize(numSamples);

    // Read m parameter from bitstream
    int decode_m = bs.readBits(16);

    // Create Golomb decoder with same approach as encoder
    Golomb golomb(decode_m, Golomb::HandleSignApproach::ODD_EVEN_MAPPING);

    // Decode residuals and reconstruct samples
    PredictorType usedPredictor;

    for (size_t i = 0; i < numSamples; i++)
    {
        int32_t residual = golomb.decode(bs);

        int32_t prediction;
        switch (predictor)
        {
        case PredictorType::LINEAR_1:
            prediction = predictLinear1(samples, i);
            break;
        case PredictorType::LINEAR_2:
            prediction = predictLinear2(samples, i);
            break;
        case PredictorType::LINEAR_3:
            prediction = predictLinear3(samples, i);
            break;
        case PredictorType::ADAPTIVE:
            prediction = predictAdaptive(samples, i, usedPredictor);
            break;
        }

        samples[i] = prediction + residual;
    }

    return true;
}

// Main encode function
bool AudioCodec::encode(const std::vector<int32_t> &audioData,
                        int channels,
                        int sampleRate,
                        int bitsPerSample,
                        const std::string &outputFile)
{
    // Open BitStream for writing
    BitStream bs(outputFile, std::ios::out | std::ios::binary);

    // Write header
    bs.writeBits(0x474F4C4D, 32); // "GOLM" magic number
    bs.writeBits(channels, 8);
    bs.writeBits(sampleRate, 32);
    bs.writeBits(bitsPerSample, 8);
    bs.writeBits(static_cast<int>(predictor), 8);
    bs.writeBits(static_cast<int>(stereoMode), 8);
    bs.writeBits(audioData.size() / channels, 32); // Samples per channel

    size_t originalSize = audioData.size() * (bitsPerSample / 8);
    int optimalM = m;

    if (channels == 1)
    {
        // Mono encoding
        encodeChannel(audioData, bs, optimalM);
    }
    else if (channels == 2)
    {
        // Stereo encoding
        std::vector<int32_t> left, right;
        for (size_t i = 0; i < audioData.size(); i += 2)
        {
            left.push_back(audioData[i]);
            right.push_back(audioData[i + 1]);
        }

        if (stereoMode == StereoMode::MID_SIDE || stereoMode == StereoMode::ADAPTIVE)
        {
            std::vector<int32_t> mid, side;
            midSideTransform(left, right, mid, side);
            bs.writeBits(1, 1); // Flag: mid-side encoded
            encodeChannel(mid, bs, optimalM);
            encodeChannel(side, bs, optimalM);
        }
        else
        {
            bs.writeBits(0, 1); // Flag: independent channels
            encodeChannel(left, bs, optimalM);
            encodeChannel(right, bs, optimalM);
        }
    }

    // Close bitstream before reading file size
    bs.close();

    // Calculate compressed size from file
    std::ifstream file(outputFile, std::ios::binary | std::ios::ate);
    size_t compressedSize = file.tellg();
    file.close();

    // Update statistics
    lastStats.originalSize = originalSize;
    lastStats.compressedSize = compressedSize;
    lastStats.compressionRatio = static_cast<double>(originalSize) / compressedSize;
    lastStats.bitsPerSample = (compressedSize * 8.0) / (audioData.size() / channels);
    lastStats.optimalM = optimalM;
    lastStats.bestPredictor = predictor;
    lastStats.bestStereoMode = stereoMode;
    lastStats.channels = channels;
    lastStats.sampleRate = sampleRate;
    lastStats.bitsPerSampleOriginal = bitsPerSample;

    return true;
}

// Main decode function
bool AudioCodec::decode(const std::string &inputFile,
                        std::vector<int32_t> &audioData,
                        int &channels,
                        int &sampleRate,
                        int &bitsPerSample)
{
    // Open BitStream for reading
    BitStream bs(inputFile, std::ios::in | std::ios::binary);

    // Read and verify header
    uint32_t magic = bs.readBits(32);
    if (magic != 0x474F4C4D)
    {
        std::cerr << "Invalid file format (magic: 0x" << std::hex << magic << ")" << std::endl;
        return false;
    }

    channels = bs.readBits(8);
    sampleRate = bs.readBits(32);
    bitsPerSample = bs.readBits(8);
    predictor = static_cast<PredictorType>(bs.readBits(8));
    stereoMode = static_cast<StereoMode>(bs.readBits(8));
    size_t samplesPerChannel = bs.readBits(32);

    if (channels == 1)
    {
        // Mono decoding
        decodeChannel(audioData, samplesPerChannel, bs);
    }
    else if (channels == 2)
    {
        // Stereo decoding
        bool midSideEncoded = bs.readBits(1);

        std::vector<int32_t> chan1, chan2;
        decodeChannel(chan1, samplesPerChannel, bs);
        decodeChannel(chan2, samplesPerChannel, bs);

        if (midSideEncoded)
        {
            std::vector<int32_t> left, right;
            inverseMidSideTransform(chan1, chan2, left, right);

            audioData.resize(samplesPerChannel * 2);
            for (size_t i = 0; i < samplesPerChannel; i++)
            {
                audioData[i * 2] = left[i];
                audioData[i * 2 + 1] = right[i];
            }
        }
        else
        {
            audioData.resize(samplesPerChannel * 2);
            for (size_t i = 0; i < samplesPerChannel; i++)
            {
                audioData[i * 2] = chan1[i];
                audioData[i * 2 + 1] = chan2[i];
            }
        }
    }

    bs.close();
    return true;
}
#include "../../includes/codec/audio_codec.hpp"
#include "../../includes/bitstream.hpp"
#include "../../includes/golomb.hpp"
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstring>

// ==================== AudioCodec Implementation ====================

AudioCodec::AudioCodec(PredictorType predictor, ChannelMode channelMode,
                       int fixedM, bool adaptiveM)
    : predictor_(predictor),
      channelMode_(channelMode),
      fixedM_(fixedM),
      adaptiveM_(adaptiveM),
      blockSize_(1024)
{
}

AudioCodec::AudioCodec()
    : predictor_(PredictorType::LINEAR2),
      channelMode_(ChannelMode::INDEPENDENT),
      fixedM_(0),
      adaptiveM_(true),
      blockSize_(1024)
{
}

void AudioCodec::setBlockSize(int blockSize)
{
    blockSize_ = blockSize;
}

AudioCodec::CompressionStats AudioCodec::getLastStats() const
{
    return lastStats_;
}

// ==================== Prediction Functions ====================

int16_t AudioCodec::predictSample(const std::vector<int16_t> &history,
                                  size_t pos, PredictorType type) const
{
    if (pos == 0)
        return 0;

    switch (type)
    {
    case PredictorType::NONE:
        return 0;

    case PredictorType::LINEAR1:
        return history[pos - 1];

    case PredictorType::LINEAR2:
        if (pos < 2)
            return history[pos - 1];
        return 2 * history[pos - 1] - history[pos - 2];

    case PredictorType::LINEAR3:
        if (pos < 3)
        {
            if (pos < 2)
                return history[pos - 1];
            return 2 * history[pos - 1] - history[pos - 2];
        }
        return 3 * history[pos - 1] - 3 * history[pos - 2] + history[pos - 3];

    case PredictorType::ADAPTIVE:
    {
        if (pos < 2)
            return history[pos - 1];
        if (pos < 3)
            return 2 * history[pos - 1] - history[pos - 2];

        // Try all predictors and choose the one with smallest absolute error
        int16_t pred1 = history[pos - 1];
        int16_t pred2 = 2 * history[pos - 1] - history[pos - 2];
        int16_t pred3 = 3 * history[pos - 1] - 3 * history[pos - 2] + history[pos - 3];

        int32_t err1 = std::abs((int32_t)history[pos - 1] - pred1);
        int32_t err2 = std::abs((int32_t)history[pos - 1] - pred2);
        int32_t err3 = std::abs((int32_t)history[pos - 1] - pred3);

        if (err1 <= err2 && err1 <= err3)
            return pred1;
        if (err2 <= err3)
            return pred2;
        return pred3;
    }

    default:
        return 0;
    }
}

// ==================== M Parameter Calculation ====================

int AudioCodec::calculateOptimalM(const std::vector<int16_t> &residuals) const
{
    if (residuals.empty())
        return 8;

    // Calculate mean absolute value of residuals
    double sum = 0;
    for (int16_t r : residuals)
    {
        sum += std::abs(r);
    }
    double mean = sum / residuals.size();

    // Optimal M is approximately 0.95 * mean (Golomb-Rice optimal parameter)
    int m = static_cast<int>(std::max(1.0, 0.95 * mean));

    // Clamp to reasonable range
    m = std::max(1, std::min(65535, m));

    return m;
}

// ==================== Channel Encoding/Decoding ====================

void AudioCodec::applyMidSideEncoding(const std::vector<int16_t> &left,
                                      const std::vector<int16_t> &right,
                                      std::vector<int16_t> &mid,
                                      std::vector<int16_t> &side) const
{
    mid.resize(left.size());
    side.resize(left.size());

    for (size_t i = 0; i < left.size(); i++)
    {
        int32_t l = left[i];
        int32_t r = right[i];
        mid[i] = (l + r) / 2;
        side[i] = l - r;
    }
}

void AudioCodec::applyMidSideDecoding(const std::vector<int16_t> &mid,
                                      const std::vector<int16_t> &side,
                                      std::vector<int16_t> &left,
                                      std::vector<int16_t> &right) const
{
    left.resize(mid.size());
    right.resize(mid.size());

    for (size_t i = 0; i < mid.size(); i++)
    {
        int32_t m = mid[i];
        int32_t s = side[i];
        left[i] = m + s / 2;
        right[i] = m - (s + 1) / 2;
    }
}

void AudioCodec::applyLeftSideEncoding(const std::vector<int16_t> &left,
                                       const std::vector<int16_t> &right,
                                       std::vector<int16_t> &leftOut,
                                       std::vector<int16_t> &side) const
{
    leftOut = left;
    side.resize(left.size());

    for (size_t i = 0; i < left.size(); i++)
    {
        side[i] = left[i] - right[i];
    }
}

void AudioCodec::applyLeftSideDecoding(const std::vector<int16_t> &leftIn,
                                       const std::vector<int16_t> &side,
                                       std::vector<int16_t> &left,
                                       std::vector<int16_t> &right) const
{
    left = leftIn;
    right.resize(leftIn.size());

    for (size_t i = 0; i < leftIn.size(); i++)
    {
        right[i] = leftIn[i] - side[i];
    }
}

// ==================== Encoding Implementation ====================

bool AudioCodec::encodeMono(const std::vector<int16_t> &samples,
                            uint32_t sampleRate, uint16_t bitsPerSample,
                            const std::string &outputFile)
{
    try
    {
        BitStream bs(outputFile, std::ios::out | std::ios::binary);

        // Write header
        bs.writeString("GOLOMB_MONO");
        bs.writeBits(sampleRate, 32);
        bs.writeBits(bitsPerSample, 16);
        bs.writeBits(samples.size(), 32);
        bs.writeBits(static_cast<uint8_t>(predictor_), 8);
        bs.writeBits(adaptiveM_ ? 1 : 0, 1);
        bs.writeBits(fixedM_, 16);
        bs.writeBits(blockSize_, 32);
        bs.flush(); // Ensure header is written

        // Encode samples in blocks
        size_t totalSamples = samples.size();
        size_t blockStart = 0;
        std::vector<int16_t> residuals;

        while (blockStart < totalSamples)
        {
            size_t blockEnd = std::min(blockStart + blockSize_, totalSamples);
            size_t blockLen = blockEnd - blockStart;

            // Calculate residuals for this block
            residuals.clear();
            residuals.reserve(blockLen);

            for (size_t i = blockStart; i < blockEnd; i++)
            {
                int16_t prediction = predictSample(samples, i, predictor_);
                int16_t residual = samples[i] - prediction;
                residuals.push_back(residual);
            }

            // Calculate optimal M for this block
            int m = adaptiveM_ ? calculateOptimalM(residuals) : fixedM_;
            if (m < 1)
                m = 1;
            m = std::min(m, 32767); // Clamp M to valid range

            // Write M parameter
            bs.writeBits(m, 16);

            // Encode residuals with Golomb coding
            Golomb golomb(m, Golomb::HandleSignApproach::ODD_EVEN_MAPPING);
            for (int16_t residual : residuals)
            {
                golomb.encode(residual, bs);
            }

            blockStart = blockEnd;
        }

        bs.close();

        // Calculate statistics
        std::ifstream compFile(outputFile, std::ios::binary | std::ios::ate);
        lastStats_.compressedSize = compFile.tellg();
        compFile.close();

        lastStats_.originalSize = samples.size() * sizeof(int16_t);
        lastStats_.compressionRatio = lastStats_.compressedSize > 0 ? (double)lastStats_.originalSize / lastStats_.compressedSize : 1.0;
        lastStats_.bitsPerSample = lastStats_.compressedSize > 0 ? (double)(lastStats_.compressedSize * 8) / samples.size() : 16.0;
        lastStats_.optimalM = adaptiveM_ ? calculateOptimalM(residuals) : fixedM_;

        return lastStats_.compressionRatio > 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Encoding error: " << e.what() << std::endl;
        return false;
    }
}

bool AudioCodec::encodeStereo(const std::vector<int16_t> &left,
                              const std::vector<int16_t> &right,
                              uint32_t sampleRate, uint16_t bitsPerSample,
                              const std::string &outputFile)
{
    try
    {
        BitStream bs(outputFile, std::ios::out | std::ios::binary);

        // Write header
        bs.writeString("GOLOMB_STEREO");
        bs.writeBits(sampleRate, 32);
        bs.writeBits(bitsPerSample, 16);
        bs.writeBits(left.size(), 32);
        bs.writeBits(static_cast<uint8_t>(predictor_), 8);
        bs.writeBits(static_cast<uint8_t>(channelMode_), 8);
        bs.writeBits(adaptiveM_ ? 1 : 0, 1);
        bs.writeBits(fixedM_, 16);
        bs.writeBits(blockSize_, 32);
        bs.flush(); // Ensure header is written

        // Apply channel coding
        std::vector<int16_t> ch1, ch2;

        switch (channelMode_)
        {
        case ChannelMode::MID_SIDE:
            applyMidSideEncoding(left, right, ch1, ch2);
            break;
        case ChannelMode::LEFT_SIDE:
            applyLeftSideEncoding(left, right, ch1, ch2);
            break;
        case ChannelMode::INDEPENDENT:
        default:
            ch1 = left;
            ch2 = right;
            break;
        }

        // Encode both channels
        encodeChannel(bs, ch1);
        encodeChannel(bs, ch2);

        bs.close();

        // Calculate statistics
        std::ifstream compFile(outputFile, std::ios::binary | std::ios::ate);
        lastStats_.compressedSize = compFile.tellg();
        compFile.close();

        lastStats_.originalSize = left.size() * 2 * sizeof(int16_t);
        lastStats_.compressionRatio = lastStats_.compressedSize > 0 ? (double)lastStats_.originalSize / lastStats_.compressedSize : 1.0;
        lastStats_.bitsPerSample = lastStats_.compressedSize > 0 ? (double)(lastStats_.compressedSize * 8) / (left.size() * 2) : 16.0;

        std::vector<int16_t> testResiduals;
        for (size_t i = 0; i < std::min(blockSize_, ch1.size()); i++)
        {
            int16_t pred = predictSample(ch1, i, predictor_);
            testResiduals.push_back(ch1[i] - pred);
        }
        lastStats_.optimalM = adaptiveM_ ? calculateOptimalM(testResiduals) : fixedM_;

        return lastStats_.compressionRatio > 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Encoding error: " << e.what() << std::endl;
        return false;
    }
}

void AudioCodec::encodeChannel(BitStream &bs, const std::vector<int16_t> &samples)
{
    size_t totalSamples = samples.size();
    size_t blockStart = 0;
    std::vector<int16_t> residuals;
    int blockNum = 0;

    std::cout << "  Encoding channel with " << totalSamples << " samples in blocks of " << blockSize_ << std::endl;

    while (blockStart < totalSamples)
    {
        size_t blockEnd = std::min(blockStart + blockSize_, totalSamples);
        size_t blockLen = blockEnd - blockStart;

        // Calculate residuals
        residuals.clear();
        residuals.reserve(blockLen);

        for (size_t i = blockStart; i < blockEnd; i++)
        {
            int16_t prediction = predictSample(samples, i, predictor_);
            int16_t residual = samples[i] - prediction;
            residuals.push_back(residual);
        }

        // Calculate optimal M
        int m = adaptiveM_ ? calculateOptimalM(residuals) : fixedM_;
        if (m < 1)
            m = 1;
        m = std::min(m, 32767);

        // Debug: Print block info
        if (blockNum % 100 == 0)
        {
            std::cout << "    Block " << blockNum << ": samples " << blockStart
                      << "-" << blockEnd << ", M=" << m
                      << ", blockLen=" << blockLen << std::endl;
        }

        // Write M parameter
        bs.writeBits(m, 16);

        // Encode residuals
        Golomb golomb(m, Golomb::HandleSignApproach::ODD_EVEN_MAPPING);
        for (int16_t residual : residuals)
        {
            golomb.encode(residual, bs);
        }

        blockStart = blockEnd;
        blockNum++;
    }

    std::cout << "  Encoded " << blockNum << " blocks, " << totalSamples << " total samples" << std::endl;

    // CRITICAL: Flush and align to byte boundary after encoding channel
    bs.flush();
    bs.alignToByte(); // Ensure next channel starts on byte boundary
}

// ==================== Decoding Implementation ====================

bool AudioCodec::decode(const std::string &inputFile,
                        std::vector<int16_t> &left, std::vector<int16_t> &right,
                        uint32_t &sampleRate, uint16_t &numChannels,
                        uint16_t &bitsPerSample)
{
    try
    {
        BitStream bs(inputFile, std::ios::in | std::ios::binary);

        // Read header
        std::string magic = bs.readString();

        if (magic == "GOLOMB_MONO")
        {
            return decodeMono(bs, left, right, sampleRate, numChannels, bitsPerSample);
        }
        else if (magic == "GOLOMB_STEREO")
        {
            return decodeStereo(bs, left, right, sampleRate, numChannels, bitsPerSample);
        }
        else
        {
            std::cerr << "Invalid file format: " << magic << std::endl;
            return false;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Decoding error: " << e.what() << std::endl;
        return false;
    }
}

bool AudioCodec::decodeMono(BitStream &bs, std::vector<int16_t> &left,
                            std::vector<int16_t> &right,
                            uint32_t &sampleRate, uint16_t &numChannels,
                            uint16_t &bitsPerSample)
{
    // Read header
    sampleRate = bs.readBits(32);
    bitsPerSample = bs.readBits(16);
    uint32_t totalSamples = bs.readBits(32);
    PredictorType predictor = static_cast<PredictorType>(bs.readBits(8));
    bool adaptiveM = bs.readBits(1);
    int fixedM = bs.readBits(16);
    size_t blockSize = bs.readBits(32);

    numChannels = 1;
    left.clear();
    left.reserve(totalSamples);
    right.clear();

    // Decode samples block by block
    size_t samplesDecoded = 0;

    while (samplesDecoded < totalSamples)
    {
        // Check if we can read more data
        if (bs.eof())
        {
            std::cerr << "Unexpected EOF: decoded " << samplesDecoded
                      << " of " << totalSamples << " samples" << std::endl;
            return false;
        }

        // Read M parameter for this block
        int m = bs.readBits(16);
        if (m < 1)
        {
            std::cerr << "Invalid M parameter: " << m << std::endl;
            return false;
        }

        // Determine block size
        size_t blockLen = std::min(blockSize, totalSamples - samplesDecoded);

        // Decode residuals
        Golomb golomb(m, Golomb::HandleSignApproach::ODD_EVEN_MAPPING);
        std::vector<int16_t> residuals;
        residuals.reserve(blockLen);

        for (size_t i = 0; i < blockLen; i++)
        {
            if (bs.eof())
            {
                std::cerr << "Unexpected EOF while reading residuals at sample "
                          << (samplesDecoded + i) << std::endl;
                return false;
            }
            int residual = golomb.decode(bs);
            residuals.push_back(static_cast<int16_t>(residual));
        }

        // Reconstruct samples from residuals
        for (size_t i = 0; i < residuals.size(); i++)
        {
            int16_t prediction = predictSample(left, left.size(), predictor);
            int16_t sample = prediction + residuals[i];
            left.push_back(sample);
        }

        samplesDecoded += blockLen;
    }

    return left.size() == totalSamples;
}

bool AudioCodec::decodeStereo(BitStream &bs, std::vector<int16_t> &left,
                              std::vector<int16_t> &right,
                              uint32_t &sampleRate, uint16_t &numChannels,
                              uint16_t &bitsPerSample)
{
    // Read header
    sampleRate = bs.readBits(32);
    bitsPerSample = bs.readBits(16);
    uint32_t totalSamples = bs.readBits(32);
    PredictorType predictor = static_cast<PredictorType>(bs.readBits(8));
    ChannelMode channelMode = static_cast<ChannelMode>(bs.readBits(8));
    bool adaptiveM = bs.readBits(1);
    int fixedM = bs.readBits(16);
    size_t blockSize = bs.readBits(32);

    numChannels = 2;

    // Decode both channels
    std::vector<int16_t> ch1, ch2;
    if (!decodeChannel(bs, ch1, totalSamples, blockSize, predictor))
        return false;
    if (!decodeChannel(bs, ch2, totalSamples, blockSize, predictor))
        return false;

    // Apply channel decoding
    switch (channelMode)
    {
    case ChannelMode::MID_SIDE:
        applyMidSideDecoding(ch1, ch2, left, right);
        break;
    case ChannelMode::LEFT_SIDE:
        applyLeftSideDecoding(ch1, ch2, left, right);
        break;
    case ChannelMode::INDEPENDENT:
    default:
        left = ch1;
        right = ch2;
        break;
    }

    return left.size() == totalSamples && right.size() == totalSamples;
}

bool AudioCodec::decodeChannel(BitStream &bs, std::vector<int16_t> &samples,
                               size_t totalSamples, size_t blockSize,
                               PredictorType predictor)
{
    // CRITICAL: Align to byte boundary before reading channel
    bs.alignToByte();

    samples.clear();
    samples.reserve(totalSamples);

    size_t samplesDecoded = 0;
    int blockNum = 0;

    std::cout << "  Decoding channel expecting " << totalSamples << " samples in blocks of " << blockSize << std::endl;

    while (samplesDecoded < totalSamples)
    {
        // Check if we can read more data
        if (bs.eof())
        {
            std::cerr << "  ERROR: Unexpected EOF at block " << blockNum
                      << ": decoded " << samplesDecoded
                      << " of " << totalSamples << " samples" << std::endl;
            return false;
        }

        // Read M parameter
        int m = bs.readBits(16);
        if (m < 1)
        {
            std::cerr << "  ERROR: Invalid M parameter in block " << blockNum
                      << ": M=" << m << std::endl;
            return false;
        }

        // Determine block size
        size_t blockLen = std::min(blockSize, totalSamples - samplesDecoded);

        // Debug: Print block info
        if (blockNum % 100 == 0)
        {
            std::cout << "    Block " << blockNum << ": expecting " << blockLen
                      << " samples, M=" << m
                      << ", total decoded so far: " << samplesDecoded << std::endl;
        }

        // Decode residuals
        Golomb golomb(m, Golomb::HandleSignApproach::ODD_EVEN_MAPPING);
        std::vector<int16_t> residuals;
        residuals.reserve(blockLen);

        for (size_t i = 0; i < blockLen; i++)
        {
            if (bs.eof())
            {
                std::cerr << "  ERROR: Unexpected EOF in block " << blockNum
                          << " while reading residual " << i
                          << " of " << blockLen
                          << " (sample " << (samplesDecoded + i) << " overall)" << std::endl;
                std::cerr << "  Decoded " << samplesDecoded << " samples successfully before error" << std::endl;
                return false;
            }
            int residual = golomb.decode(bs);
            residuals.push_back(static_cast<int16_t>(residual));
        }

        // Reconstruct samples
        for (size_t i = 0; i < residuals.size(); i++)
        {
            int16_t prediction = predictSample(samples, samples.size(), predictor);
            int16_t sample = prediction + residuals[i];
            samples.push_back(sample);
        }

        samplesDecoded += blockLen;
        blockNum++;
    }

    std::cout << "  Successfully decoded " << blockNum << " blocks, " << samplesDecoded << " total samples" << std::endl;
    return samples.size() == totalSamples;
}
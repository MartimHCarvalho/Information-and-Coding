#include "../../includes/codec/image_codec.hpp"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits>
#include <sys/stat.h>

ImageCodec::ImageCodec(PredictorType pred, int m, bool adaptM)
    : predictorType(pred), initialM(m), adaptiveM(adaptM)
{
    lastStats = {0, 0, 0.0, 0.0, 0, PredictorType::NONE};
}

int ImageCodec::calculateOptimalM(const std::vector<int> &residuals)
{
    if (residuals.empty())
        return 4;

    // OPTIMIZATION: More aggressive sampling - use max 2000 samples
    size_t sampleSize = std::min(residuals.size(), size_t(2000));
    size_t step = residuals.size() / sampleSize;
    if (step == 0)
        step = 1;

    // Use integer arithmetic for speed
    int64_t sum = 0;
    size_t count = 0;
    for (size_t i = 0; i < residuals.size(); i += step)
    {
        sum += std::abs(residuals[i]);
        count++;
    }
    int mean = static_cast<int>(sum / count);

    // Simplified calculation
    int m = (mean * 7) / 10; // Faster than 0.7 multiplication
    m = std::max(1, std::min(m, 256));

    return m;
}

// OPTIMIZATION: Inline prediction helpers for decode fast path
inline int predictLeft(const std::vector<uint8_t> &image, int idx, int x)
{
    return (x > 0) ? image[idx - 1] : 128;
}

inline int predictTop(const std::vector<uint8_t> &image, int idx, int width, int y)
{
    return (y > 0) ? image[idx - width] : 128;
}

inline int predictJPEGLS(const std::vector<uint8_t> &image, int idx, int width, int x, int y)
{
    if (x == 0)
        return (y > 0) ? image[idx - width] : 128;
    if (y == 0)
        return image[idx - 1];

    const int left = image[idx - 1];
    const int top = image[idx - width];
    const int topLeft = image[idx - width - 1];
    const int minVal = std::min(left, top);
    const int maxVal = std::max(left, top);

    if (topLeft >= maxVal)
        return minVal;
    else if (topLeft <= minVal)
        return maxVal;
    else
        return left + top - topLeft;
}

inline int predictPaeth(const std::vector<uint8_t> &image, int idx, int width, int x, int y)
{
    if (x == 0)
        return (y > 0) ? image[idx - width] : 128;
    if (y == 0)
        return image[idx - 1];

    const int left = image[idx - 1];
    const int top = image[idx - width];
    const int topLeft = image[idx - width - 1];
    const int p = left + top - topLeft;
    const int pa = std::abs(p - left);
    const int pb = std::abs(p - top);
    const int pc = std::abs(p - topLeft);

    if (pa <= pb && pa <= pc)
        return left;
    else if (pb <= pc)
        return top;
    else
        return topLeft;
}

inline int predictAverage(const std::vector<uint8_t> &image, int idx, int width, int x, int y)
{
    if (x == 0)
        return (y > 0) ? image[idx - width] : 128;
    if (y == 0)
        return image[idx - 1];
    const int left = image[idx - 1];
    const int top = image[idx - width];
    return (left + top) >> 1;
}

int ImageCodec::predictPixel(const std::vector<uint8_t> &image,
                             int width, int /*height*/,
                             int x, int y, PredictorType pred)
{
    const int idx = y * width + x;

    if (x == 0 && y == 0)
        return 128;

    switch (pred)
    {
    case PredictorType::NONE:
        return 0;

    case PredictorType::LEFT:
        return predictLeft(image, idx, x);

    case PredictorType::TOP:
        return predictTop(image, idx, width, y);

    case PredictorType::AVERAGE:
        return predictAverage(image, idx, width, x, y);

    case PredictorType::PAETH:
        return predictPaeth(image, idx, width, x, y);

    case PredictorType::JPEGLS:
        return predictJPEGLS(image, idx, width, x, y);

    case PredictorType::ADAPTIVE:
        return predictLeft(image, idx, x);

    default:
        return predictLeft(image, idx, x);
    }
}

double ImageCodec::evaluatePredictor(const std::vector<uint8_t> &image,
                                     int width, int height, int row,
                                     PredictorType pred)
{
    if (row >= height)
        return std::numeric_limits<double>::max();

    // OPTIMIZATION: More aggressive sampling - every 8th pixel
    int64_t errorSum = 0;
    int count = 0;
    const int step = std::max(8, width / 32); // Reduced evaluations
    const int rowOffset = row * width;

    for (int x = 0; x < width; x += step)
    {
        const int actual = image[rowOffset + x];
        const int predicted = predictPixel(image, width, height, x, row, pred);
        errorSum += std::abs(actual - predicted);
        count++;
    }

    return count > 0 ? static_cast<double>(errorSum) / count : 0.0;
}

ImageCodec::PredictorType ImageCodec::selectBestPredictor(
    const std::vector<uint8_t> &image, int width, int height, int row)
{

    if (row == 0)
        return PredictorType::LEFT;

    // OPTIMIZATION: Only test 2 predictors for speed
    static const PredictorType predictors[] = {
        PredictorType::JPEGLS,
        PredictorType::PAETH};

    PredictorType best = PredictorType::JPEGLS;
    double minError = evaluatePredictor(image, width, height, row - 1, predictors[0]);

    // Early exit threshold
    if (minError < 2.0)
        return best;

    double error = evaluatePredictor(image, width, height, row - 1, predictors[1]);
    if (error < minError)
    {
        best = predictors[1];
    }

    return best;
}

bool ImageCodec::encode(const std::vector<uint8_t> &image,
                        int width, int height,
                        const std::string &outputFile)
{
    if (image.size() != static_cast<size_t>(width * height))
    {
        std::cerr << "Image size mismatch" << std::endl;
        return false;
    }

    BitStream bs(outputFile, std::ios::out | std::ios::binary);

    // Write header
    bs.writeBits(width, 32);
    bs.writeBits(height, 32);
    bs.writeBits(255, 8);
    bs.writeBits(static_cast<int>(predictorType), 8);

    // OPTIMIZATION: Faster m estimation with fewer sample rows
    int m = initialM;
    if (m <= 0)
    {
        int sampleRows = std::min(16, height); // Reduced from 32
        std::vector<int> sampleResiduals;
        sampleResiduals.reserve(sampleRows * width);

        PredictorType currentPredictor = predictorType;
        for (int y = 0; y < sampleRows; y++)
        {
            if (predictorType == PredictorType::ADAPTIVE)
            {
                currentPredictor = selectBestPredictor(image, width, height, y);
            }

            const int rowOffset = y * width;
            for (int x = 0; x < width; x++)
            {
                const int actual = image[rowOffset + x];
                const int predicted = predictPixel(image, width, height, x, y, currentPredictor);
                sampleResiduals.push_back(actual - predicted);
            }
        }
        m = calculateOptimalM(sampleResiduals);
    }

    bs.writeBits(m, 16);

    std::cout << "Using m = " << m << " for Golomb encoding" << std::endl;
    lastStats.optimalM = m;
    lastStats.usedPredictor = predictorType;

    // Create Golomb encoder
    Golomb golomb(m, Golomb::HandleSignApproach::ODD_EVEN_MAPPING);

    // OPTIMIZATION: Disable adaptive m by default for speed
    const bool useAdaptiveM = adaptiveM && height > 512;

    std::vector<int> rowResiduals;
    if (useAdaptiveM)
    {
        rowResiduals.reserve(width * 128);
    }

    PredictorType currentPredictor = predictorType;

    // OPTIMIZATION: Cache predictor selection for multiple rows
    const int predictorCacheRows = (predictorType == PredictorType::ADAPTIVE) ? 4 : 0;
    PredictorType cachedPredictor = currentPredictor;
    int cacheCounter = 0;

    for (int y = 0; y < height; y++)
    {
        // Update predictor less frequently
        if (predictorType == PredictorType::ADAPTIVE)
        {
            if (cacheCounter == 0)
            {
                cachedPredictor = selectBestPredictor(image, width, height, y);
                cacheCounter = predictorCacheRows;
            }
            currentPredictor = cachedPredictor;
            cacheCounter--;

            bs.writeBits(static_cast<int>(currentPredictor), 8);
        }

        // OPTIMIZATION: Unroll loop and process multiple pixels at once
        const int rowOffset = y * width;
        int x = 0;

        // Process 4 pixels at a time when possible
        for (; x + 3 < width; x += 4)
        {
            for (int i = 0; i < 4; i++)
            {
                const int actual = image[rowOffset + x + i];
                const int predicted = predictPixel(image, width, height, x + i, y, currentPredictor);
                const int residual = actual - predicted;
                golomb.encode(residual, bs);

                if (useAdaptiveM)
                {
                    rowResiduals.push_back(residual);
                }
            }
        }

        // Handle remaining pixels
        for (; x < width; x++)
        {
            const int actual = image[rowOffset + x];
            const int predicted = predictPixel(image, width, height, x, y, currentPredictor);
            const int residual = actual - predicted;
            golomb.encode(residual, bs);

            if (useAdaptiveM)
            {
                rowResiduals.push_back(residual);
            }
        }

        // OPTIMIZATION: Even less frequent m adaptation (every 128 rows)
        if (useAdaptiveM && y % 128 == 127 && y < height - 1)
        {
            int newM = calculateOptimalM(rowResiduals);
            // Only update if change is significant (>20%)
            if (std::abs(newM - m) > m / 5)
            {
                bs.writeBit(1);
                bs.writeBits(newM, 16);
                m = newM;
                golomb.setM(m);
            }
            else
            {
                bs.writeBit(0);
            }
            rowResiduals.clear();
            if (y + 128 < height)
            {
                rowResiduals.reserve(width * 128);
            }
        }
    }

    bs.close();

    // Get file size for statistics
    struct stat st;
    if (stat(outputFile.c_str(), &st) == 0)
    {
        lastStats.compressedSize = st.st_size;
    }
    else
    {
        lastStats.compressedSize = 0;
    }

    // Calculate statistics
    lastStats.originalSize = width * height;
    lastStats.compressionRatio = static_cast<double>(lastStats.originalSize) /
                                 lastStats.compressedSize;
    lastStats.bitsPerPixel = (8.0 * lastStats.compressedSize) /
                             (width * height);

    std::cout << "Compression complete:" << std::endl;
    std::cout << "  Original: " << lastStats.originalSize << " bytes" << std::endl;
    std::cout << "  Compressed: " << lastStats.compressedSize << " bytes" << std::endl;
    std::cout << "  Ratio: " << lastStats.compressionRatio << ":1" << std::endl;
    std::cout << "  Bits per pixel: " << lastStats.bitsPerPixel << std::endl;

    return true;
}

bool ImageCodec::decode(const std::string &inputFile,
                        std::vector<uint8_t> &image,
                        int &width, int &height)
{
    BitStream bs(inputFile, std::ios::in | std::ios::binary);

    // Read header
    width = bs.readBits(32);
    height = bs.readBits(32);
    int maxValue = bs.readBits(8);
    PredictorType decodePred = static_cast<PredictorType>(bs.readBits(8));
    int m = bs.readBits(16);

    if (width <= 0 || height <= 0 || maxValue != 255)
    {
        std::cerr << "Invalid image header" << std::endl;
        return false;
    }

    image.resize(width * height);

    // Create Golomb decoder
    Golomb golomb(m, Golomb::HandleSignApproach::ODD_EVEN_MAPPING);

    const bool useAdaptiveM = adaptiveM && height > 512;

// OPTIMIZATION 1: True branchless clamping macro
// This compiles to efficient conditional moves (CMOV on x86)
#define CLAMP_FAST(val) \
    (static_cast<uint8_t>((val) & ~((val) >> 31) & (255 - ((val) - 256) >> 31)))

// Alternative branchless clamp that's often faster:
#define CLAMP_BYTE(val) \
    (static_cast<uint8_t>((val) < 0 ? 0 : ((val) > 255 ? 255 : (val))))

    // OPTIMIZATION 2: Fast path for non-adaptive predictors
    if (decodePred != PredictorType::ADAPTIVE)
    {
        // Pre-compute predictor-specific constants
        const bool isLeft = (decodePred == PredictorType::LEFT);
        const bool isTop = (decodePred == PredictorType::TOP);
        const bool isJPEGLS = (decodePred == PredictorType::JPEGLS);
        const bool isPaeth = (decodePred == PredictorType::PAETH);
        const bool isAverage = (decodePred == PredictorType::AVERAGE);

        for (int y = 0; y < height; y++)
        {
            const int rowOffset = y * width;

            // OPTIMIZATION 3: Specialized innermost loops - no switch statements
            if (isLeft)
            {
                // First pixel in row
                int residual = golomb.decode(bs);
                int value = 128 + residual;
                image[rowOffset] = CLAMP_BYTE(value);

                // Rest of row - can use previous pixel directly
                for (int x = 1; x < width; x++)
                {
                    const int idx = rowOffset + x;
                    residual = golomb.decode(bs);
                    value = image[idx - 1] + residual;
                    image[idx] = CLAMP_BYTE(value);
                }
            }
            else if (isTop)
            {
                if (y == 0)
                {
                    // First row
                    for (int x = 0; x < width; x++)
                    {
                        const int idx = rowOffset + x;
                        int residual = golomb.decode(bs);
                        int value = 128 + residual;
                        image[idx] = CLAMP_BYTE(value);
                    }
                }
                else
                {
                    // Can use pixel directly above
                    for (int x = 0; x < width; x++)
                    {
                        const int idx = rowOffset + x;
                        int residual = golomb.decode(bs);
                        int value = image[idx - width] + residual;
                        image[idx] = CLAMP_BYTE(value);
                    }
                }
            }
            else if (isJPEGLS)
            {
                for (int x = 0; x < width; x++)
                {
                    const int idx = rowOffset + x;
                    int residual = golomb.decode(bs);
                    int predicted;

                    if (x == 0)
                    {
                        predicted = (y > 0) ? image[idx - width] : 128;
                    }
                    else if (y == 0)
                    {
                        predicted = image[idx - 1];
                    }
                    else
                    {
                        // Inlined JPEGLS prediction
                        const int left = image[idx - 1];
                        const int top = image[idx - width];
                        const int topLeft = image[idx - width - 1];

                        // OPTIMIZATION 4: Compute min/max without branches
                        const int minVal = (left < top) ? left : top;
                        const int maxVal = (left > top) ? left : top;

                        if (topLeft >= maxVal)
                            predicted = minVal;
                        else if (topLeft <= minVal)
                            predicted = maxVal;
                        else
                            predicted = left + top - topLeft;
                    }

                    int value = predicted + residual;
                    image[idx] = CLAMP_BYTE(value);
                }
            }
            else if (isPaeth)
            {
                // OPTIMIZATION 1: Handle first row separately (simple left predictor)
                if (y == 0)
                {
                    int residual = golomb.decode(bs);
                    int value = 128 + residual;
                    image[rowOffset] = CLAMP_BYTE(value);

                    // Unroll loop by 4 for better instruction pipelining
                    int x = 1;
                    for (; x + 3 < width; x += 4)
                    {
                        const int idx = rowOffset + x;
                        int res0 = golomb.decode(bs);
                        int res1 = golomb.decode(bs);
                        int res2 = golomb.decode(bs);
                        int res3 = golomb.decode(bs);

                        image[idx + 0] = CLAMP_BYTE(image[idx - 1] + res0);
                        image[idx + 1] = CLAMP_BYTE(image[idx + 0] + res1);
                        image[idx + 2] = CLAMP_BYTE(image[idx + 1] + res2);
                        image[idx + 3] = CLAMP_BYTE(image[idx + 2] + res3);
                    }

                    // Handle remaining pixels
                    for (; x < width; x++)
                    {
                        const int idx = rowOffset + x;
                        residual = golomb.decode(bs);
                        image[idx] = CLAMP_BYTE(image[idx - 1] + residual);
                    }
                }
                else
                {
                    // OPTIMIZATION 2: First pixel in row (use top predictor)
                    int residual = golomb.decode(bs);
                    int value = image[rowOffset - width] + residual;
                    image[rowOffset] = CLAMP_BYTE(value);

                    // OPTIMIZATION 3: Prefetch and unroll main loop
                    int x = 1;

                    // Process 2 pixels at a time (2-way unroll for balance)
                    for (; x + 1 < width; x += 2)
                    {
                        const int idx0 = rowOffset + x;
                        const int idx1 = rowOffset + x + 1;

                        // Prefetch upcoming data
                        if (x + 8 < width)
                        {
                            __builtin_prefetch(&image[idx0 + 8], 0, 3);
                            __builtin_prefetch(&image[idx0 - width + 8], 0, 3);
                        }

                        // === PIXEL 0 ===
                        residual = golomb.decode(bs);

                        const int left0 = image[idx0 - 1];
                        const int top0 = image[idx0 - width];
                        const int topLeft0 = image[idx0 - width - 1];

                        // Branchless Paeth using ternary operators (compiles to CMOV)
                        const int p0 = left0 + top0 - topLeft0;
                        const int pa0 = std::abs(p0 - left0);
                        const int pb0 = std::abs(p0 - top0);
                        const int pc0 = std::abs(p0 - topLeft0);

                        // Branchless selection chain
                        const int pred0 = (pa0 <= pb0 && pa0 <= pc0) ? left0 : ((pb0 <= pc0) ? top0 : topLeft0);

                        image[idx0] = CLAMP_BYTE(pred0 + residual);

                        // === PIXEL 1 ===
                        residual = golomb.decode(bs);

                        const int left1 = image[idx1 - 1]; // This is the pixel we just decoded
                        const int top1 = image[idx1 - width];
                        const int topLeft1 = image[idx1 - width - 1];

                        const int p1 = left1 + top1 - topLeft1;
                        const int pa1 = std::abs(p1 - left1);
                        const int pb1 = std::abs(p1 - top1);
                        const int pc1 = std::abs(p1 - topLeft1);

                        const int pred1 = (pa1 <= pb1 && pa1 <= pc1) ? left1 : ((pb1 <= pc1) ? top1 : topLeft1);

                        image[idx1] = CLAMP_BYTE(pred1 + residual);
                    }

                    // Handle remaining pixel if width is odd
                    if (x < width)
                    {
                        const int idx = rowOffset + x;
                        residual = golomb.decode(bs);

                        const int left = image[idx - 1];
                        const int top = image[idx - width];
                        const int topLeft = image[idx - width - 1];
                        const int p = left + top - topLeft;

                        const int pa = std::abs(p - left);
                        const int pb = std::abs(p - top);
                        const int pc = std::abs(p - topLeft);

                        const int pred = (pa <= pb && pa <= pc) ? left : ((pb <= pc) ? top : topLeft);

                        image[idx] = CLAMP_BYTE(pred + residual);
                    }
                }
            }
            else if (isAverage)
            {
                for (int x = 0; x < width; x++)
                {
                    const int idx = rowOffset + x;
                    int residual = golomb.decode(bs);
                    int predicted;

                    if (x == 0)
                    {
                        predicted = (y > 0) ? image[idx - width] : 128;
                    }
                    else if (y == 0)
                    {
                        predicted = image[idx - 1];
                    }
                    else
                    {
                        // OPTIMIZATION 6: Use shift for division by 2
                        predicted = (image[idx - 1] + image[idx - width]) >> 1;
                    }

                    int value = predicted + residual;
                    image[idx] = CLAMP_BYTE(value);
                }
            }
            else
            {
                // Generic fallback
                for (int x = 0; x < width; x++)
                {
                    const int idx = rowOffset + x;
                    int residual = golomb.decode(bs);
                    int predicted = predictPixel(image, width, height, x, y, decodePred);
                    int value = predicted + residual;
                    image[idx] = CLAMP_BYTE(value);
                }
            }

            // Check for m updates (less frequently checked)
            if (useAdaptiveM && y % 128 == 127 && y < height - 1 && !bs.eof())
            {
                int updateFlag = bs.readBit();
                if (updateFlag)
                {
                    m = bs.readBits(16);
                    golomb.setM(m);
                }
            }
        }
    }
    else
    {
        // ADAPTIVE mode
        for (int y = 0; y < height; y++)
        {
            PredictorType currentPredictor = static_cast<PredictorType>(bs.readBits(8));
            const int rowOffset = y * width;

            // Same optimization as above but check current predictor
            const bool isJPEGLS = (currentPredictor == PredictorType::JPEGLS);
            const bool isPaeth = (currentPredictor == PredictorType::PAETH);

            if (isJPEGLS)
            {
                for (int x = 0; x < width; x++)
                {
                    const int idx = rowOffset + x;
                    int residual = golomb.decode(bs);
                    int predicted = predictJPEGLS(image, idx, width, x, y);
                    int value = predicted + residual;
                    image[idx] = CLAMP_BYTE(value);
                }
            }
            else if (isPaeth)
            {
                for (int x = 0; x < width; x++)
                {
                    const int idx = rowOffset + x;
                    int residual = golomb.decode(bs);
                    int predicted = predictPaeth(image, idx, width, x, y);
                    int value = predicted + residual;
                    image[idx] = CLAMP_BYTE(value);
                }
            }
            else
            {
                for (int x = 0; x < width; x++)
                {
                    const int idx = rowOffset + x;
                    int residual = golomb.decode(bs);
                    int predicted = predictPixel(image, width, height, x, y, currentPredictor);
                    int value = predicted + residual;
                    image[idx] = CLAMP_BYTE(value);
                }
            }

            if (useAdaptiveM && y % 128 == 127 && y < height - 1 && !bs.eof())
            {
                int updateFlag = bs.readBit();
                if (updateFlag)
                {
                    m = bs.readBits(16);
                    golomb.setM(m);
                }
            }
        }
    }

    bs.close();

    std::cout << "Decompression complete: " << width << "x" << height << std::endl;

    return true;
}
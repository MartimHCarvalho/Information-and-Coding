#include "image_codec.hpp"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits>
#include <sys/stat.h>

ImageCodec::ImageCodec(PredictorType pred, int m, bool adaptM)
    : predictorType(pred), initialM(m), adaptiveM(adaptM) {
    lastStats = {0, 0, 0.0, 0.0, 0, PredictorType::NONE};
}

int ImageCodec::calculateOptimalM(const std::vector<int>& residuals) {
    if (residuals.empty()) return 4;
    
    // Calculate mean absolute value of residuals
    double sum = 0.0;
    for (int r : residuals) {
        sum += std::abs(r);
    }
    double mean = sum / residuals.size();
    
    // Optimal m for geometric distribution is approximately mean * ln(2)
    // For Laplacian distribution (common in image residuals): m ≈ mean / 0.5
    int m = static_cast<int>(std::ceil(mean * 0.7));
    
    // Clamp to reasonable range
    m = std::max(1, std::min(m, 256));
    
    return m;
}

int ImageCodec::predictPixel(const std::vector<uint8_t>& image, 
                             int width, int /*height*/, 
                             int x, int y, PredictorType pred) {
    if (x == 0 && y == 0) return 128; // Default for first pixel
    
    int left = (x > 0) ? image[y * width + x - 1] : 128;
    int top = (y > 0) ? image[(y - 1) * width + x] : 128;
    int topLeft = (x > 0 && y > 0) ? image[(y - 1) * width + x - 1] : 128;
    
    switch (pred) {
        case PredictorType::NONE:
            return 0;
            
        case PredictorType::LEFT:
            return left;
            
        case PredictorType::TOP:
            return top;
            
        case PredictorType::AVERAGE:
            if (x == 0) return top;
            if (y == 0) return left;
            return (left + top) / 2;
            
        case PredictorType::PAETH: {
            // PNG Paeth predictor
            if (x == 0) return top;
            if (y == 0) return left;
            
            int p = left + top - topLeft;
            int pa = std::abs(p - left);
            int pb = std::abs(p - top);
            int pc = std::abs(p - topLeft);
            
            if (pa <= pb && pa <= pc) return left;
            else if (pb <= pc) return top;
            else return topLeft;
        }
        
        case PredictorType::JPEGLS: {
            // JPEG-LS median edge detector (MED)
            if (x == 0) return top;
            if (y == 0) return left;
            
            int minVal = std::min(left, top);
            int maxVal = std::max(left, top);
            
            if (topLeft >= maxVal) return minVal;
            else if (topLeft <= minVal) return maxVal;
            else return left + top - topLeft;
        }
        
        case PredictorType::ADAPTIVE:
            // Should not reach here - adaptive uses other predictors
            return left;
            
        default:
            return left;
    }
}

double ImageCodec::evaluatePredictor(const std::vector<uint8_t>& image, 
                                     int width, int height, int row,
                                     PredictorType pred) {
    if (row >= height) return std::numeric_limits<double>::max();
    
    double errorSum = 0.0;
    int count = 0;
    
    // Evaluate on current row
    for (int x = 0; x < width; x++) {
        int actual = image[row * width + x];
        int predicted = predictPixel(image, width, height, x, row, pred);
        int residual = actual - predicted;
        errorSum += std::abs(residual);
        count++;
    }
    
    return count > 0 ? errorSum / count : 0.0;
}

ImageCodec::PredictorType ImageCodec::selectBestPredictor(
    const std::vector<uint8_t>& image, int width, int height, int row) {
    
    if (row == 0) return PredictorType::LEFT; // First row uses left predictor
    
    std::vector<PredictorType> predictors = {
        PredictorType::LEFT,
        PredictorType::AVERAGE,
        PredictorType::PAETH,
        PredictorType::JPEGLS
    };
    
    PredictorType best = PredictorType::JPEGLS;
    double minError = std::numeric_limits<double>::max();
    
    // Use previous row to estimate which predictor works best
    for (auto pred : predictors) {
        double error = evaluatePredictor(image, width, height, row - 1, pred);
        if (error < minError) {
            minError = error;
            best = pred;
        }
    }
    
    return best;
}

bool ImageCodec::encode(const std::vector<uint8_t>& image, 
                       int width, int height,
                       const std::string& outputFile) {
    if (image.size() != static_cast<size_t>(width * height)) {
        std::cerr << "Image size mismatch" << std::endl;
        return false;
    }
    
    BitStream bs(outputFile, std::ios::out | std::ios::binary);
    
    // Write header
    bs.writeBits(width, 32);
    bs.writeBits(height, 32);
    bs.writeBits(255, 8); // maxValue (always 255 for 8-bit grayscale)
    bs.writeBits(static_cast<int>(predictorType), 8);
    
    // Collect residuals for optimal m calculation
    std::vector<int> allResiduals;
    allResiduals.reserve(width * height);
    
    // Calculate all residuals first if using adaptive M globally
    PredictorType currentPredictor = predictorType;
    
    for (int y = 0; y < height; y++) {
        if (predictorType == PredictorType::ADAPTIVE) {
            currentPredictor = selectBestPredictor(image, width, height, y);
        }
        
        for (int x = 0; x < width; x++) {
            int actual = image[y * width + x];
            int predicted = predictPixel(image, width, height, x, y, currentPredictor);
            int residual = actual - predicted;
            allResiduals.push_back(residual);
        }
    }
    
    // Calculate optimal m
    int m = (initialM > 0) ? initialM : calculateOptimalM(allResiduals);
    bs.writeBits(m, 16);
    
    std::cout << "Using m = " << m << " for Golomb encoding" << std::endl;
    lastStats.optimalM = m;
    lastStats.usedPredictor = predictorType;
    
    // Create Golomb encoder
    Golomb golomb(m, Golomb::HandleSignApproach::ODD_EVEN_MAPPING);
    
    // Encode image
    int residualIdx = 0;
    for (int y = 0; y < height; y++) {
        if (predictorType == PredictorType::ADAPTIVE) {
            currentPredictor = selectBestPredictor(image, width, height, y);
            bs.writeBits(static_cast<int>(currentPredictor), 8);
        }
        
        for (int x = 0; x < width; x++) {
            int residual = allResiduals[residualIdx++];
            golomb.encode(residual, bs);
        }
        
        // Optionally adapt m per row for better compression
        if (adaptiveM && y % 32 == 31 && y < height - 1) {
            // Recalculate m every 32 rows
            std::vector<int> recentResiduals(
                allResiduals.begin() + (y - 31) * width,
                allResiduals.begin() + (y + 1) * width
            );
            int newM = calculateOptimalM(recentResiduals);
            if (newM != m) {
                bs.writeBit(1); // Flag for m update
                bs.writeBits(newM, 16);
                m = newM;
                golomb.setM(m);
            } else {
                bs.writeBit(0); // No m update
            }
        }
    }
    
    bs.close();
    
    // Get file size for statistics
    struct stat st;
    if (stat(outputFile.c_str(), &st) == 0) {
        lastStats.compressedSize = st.st_size;
    } else {
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

bool ImageCodec::decode(const std::string& inputFile,
                       std::vector<uint8_t>& image,
                       int& width, int& height) {
    BitStream bs(inputFile, std::ios::in | std::ios::binary);
    
    // Read header
    width = bs.readBits(32);
    height = bs.readBits(32);
    int maxValue = bs.readBits(8);
    PredictorType decodePred = static_cast<PredictorType>(bs.readBits(8));
    int m = bs.readBits(16);
    
    if (width <= 0 || height <= 0 || maxValue != 255) {
        std::cerr << "Invalid image header" << std::endl;
        return false;
    }
    
    image.resize(width * height);
    
    // Create Golomb decoder
    Golomb golomb(m, Golomb::HandleSignApproach::ODD_EVEN_MAPPING);
    
    // Decode image
    PredictorType currentPredictor = decodePred;
    
    for (int y = 0; y < height; y++) {
        if (decodePred == PredictorType::ADAPTIVE) {
            currentPredictor = static_cast<PredictorType>(bs.readBits(8));
        }
        
        for (int x = 0; x < width; x++) {
            int residual = golomb.decode(bs);
            int predicted = predictPixel(image, width, height, x, y, currentPredictor);
            int value = predicted + residual;
            
            // Clamp to valid range
            value = std::max(0, std::min(255, value));
            image[y * width + x] = static_cast<uint8_t>(value);
        }
        
        // Check for m updates
        if (adaptiveM && y % 32 == 31 && y < height - 1 && !bs.eof()) {
            int updateFlag = bs.readBit();
            if (updateFlag) {
                m = bs.readBits(16);
                golomb.setM(m);
            }
        }
    }
    
    bs.close();
    
    std::cout << "Decompression complete: " << width << "x" << height << std::endl;
    
    return true;
}
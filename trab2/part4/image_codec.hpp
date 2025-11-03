#ifndef IMAGE_CODEC_HPP
#define IMAGE_CODEC_HPP

#include "../includes/golomb.hpp"
#include "../includes/bitstream.hpp"
#include <vector>
#include <string>
#include <cstdint>

class ImageCodec {
public:
    enum class PredictorType {
        NONE,           // No prediction (raw values)
        LEFT,           // Use left pixel
        TOP,            // Use top pixel
        AVERAGE,        // Average of left and top
        PAETH,          // PNG Paeth predictor
        JPEGLS,         // JPEG-LS median edge detector (best for most images)
        ADAPTIVE        // Adaptive selection per row
    };

private:
    struct ImageHeader {
        uint32_t width;
        uint32_t height;
        uint8_t maxValue;
        PredictorType predictor;
        int golombM;
    };

    PredictorType predictorType;
    int initialM;
    bool adaptiveM;
    
    // Calculate optimal m parameter based on residual statistics
    int calculateOptimalM(const std::vector<int>& residuals);
    
    // Predictor functions
    int predictPixel(const std::vector<uint8_t>& image, int width, int height, 
                     int x, int y, PredictorType pred);
    
    // Adaptive predictor selection
    PredictorType selectBestPredictor(const std::vector<uint8_t>& image, 
                                      int width, int height, int row);
    
    // Estimate predictor performance
    double evaluatePredictor(const std::vector<uint8_t>& image, 
                            int width, int height, int row, 
                            PredictorType pred);

public:
    ImageCodec(PredictorType pred = PredictorType::JPEGLS, 
               int m = 0, bool adaptM = true);
    
    // Encode grayscale image to compressed bitstream
    bool encode(const std::vector<uint8_t>& image, int width, int height,
                const std::string& outputFile);
    
    // Decode compressed bitstream to grayscale image
    bool decode(const std::string& inputFile, 
                std::vector<uint8_t>& image, int& width, int& height);
    
    // Get compression statistics
    struct CompressionStats {
        size_t originalSize;
        size_t compressedSize;
        double compressionRatio;
        double bitsPerPixel;
        int optimalM;
        PredictorType usedPredictor;
    };
    
    CompressionStats getLastStats() const { return lastStats; }
    
private:
    CompressionStats lastStats;
};

#endif
#include "codec/losslessImage.hpp"
#include "bitstream.hpp"
#include "golomb.hpp"
#include <iostream>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <fstream>

LosslessImage::LosslessImage(PredictorType predictor, int initialM, bool adaptiveM)
    : predictor(predictor), initialM(initialM), adaptiveM(adaptiveM),
      originalSize(0), compressedSize(0) {
    if (initialM <= 0) {
        throw std::invalid_argument("Initial M must be greater than 0.");
    }
}

LosslessImage::~LosslessImage() {}

int LosslessImage::clipValue(int value) const {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return value;
}

int LosslessImage::predictPixel(const cv::Mat& image, int x, int y) const {
    int prediction = 0;

    // Get neighboring pixels
    int left = (x > 0) ? image.at<uchar>(y, x - 1) : 128;
    int top = (y > 0) ? image.at<uchar>(y - 1, x) : 128;
    int topLeft = (x > 0 && y > 0) ? image.at<uchar>(y - 1, x - 1) : 128;

    switch (predictor) {
        case PredictorType::NONE:
            prediction = 128;  // Mid-range default
            break;

        case PredictorType::LEFT:
            prediction = left;
            break;

        case PredictorType::TOP:
            prediction = top;
            break;

        case PredictorType::AVERAGE:
            prediction = (left + top) / 2;
            break;

        case PredictorType::JPEGLS:
            // JPEG-LS predictor with edge detection
            if (topLeft >= std::max(left, top)) {
                prediction = std::min(left, top);
            } else if (topLeft <= std::min(left, top)) {
                prediction = std::max(left, top);
            } else {
                prediction = left + top - topLeft;
            }
            break;
    }

    return clipValue(prediction);
}

int LosslessImage::calculateAdaptiveM(const std::vector<int>& recentResiduals) const {
    if (recentResiduals.empty()) {
        return initialM;
    }

    // Calculate mean of absolute residuals
    double sumAbs = 0.0;
    for (int residual : recentResiduals) {
        sumAbs += std::abs(residual);
    }
    double meanAbs = sumAbs / recentResiduals.size();

    // Estimate parameter for geometric distribution
    // For geometric distribution: E[|X|] ≈ (1-α)/α where α = (m-1)/m
    // Solving for m: m ≈ meanAbs + 1
    int m = static_cast<int>(meanAbs + 1.0);

    // Clamp m to reasonable range
    if (m < 2) m = 2;
    if (m > 256) m = 256;

    return m;
}

void LosslessImage::updateResidualHistory(std::vector<int>& history, 
                                         int residual, 
                                         size_t maxSize) const {
    history.push_back(residual);
    if (history.size() > maxSize) {
        history.erase(history.begin());
    }
}

int LosslessImage::selectContextM(int gradientH, int gradientV) const {
    // Context-based M selection based on gradient magnitude
    int gradientMag = std::abs(gradientH) + std::abs(gradientV);

    if (gradientMag < 10) {
        return 4;  // Smooth region - small residuals expected
    } else if (gradientMag < 30) {
        return 16;  // Medium texture
    } else if (gradientMag < 60) {
        return 32;  // High texture
    } else {
        return 64;  // Edge region - large residuals possible
    }
}

bool LosslessImage::encode(const std::string& inputImagePath, 
                          const std::string& outputCompressedPath) {
    try {
        // Read grayscale image
        cv::Mat image = cv::imread(inputImagePath, cv::IMREAD_GRAYSCALE);
        if (image.empty()) {
            std::cerr << "Error: Could not read image " << inputImagePath << std::endl;
            return false;
        }

        int height = image.rows;
        int width = image.cols;
        originalSize = height * width;

        std::cout << "Encoding image: " << width << "x" << height << std::endl;

        // Create bitstream for output
        BitStream bs(outputCompressedPath, std::ios::out);

        // Write header
        bs.writeBits(width, 32);
        bs.writeBits(height, 32);
        bs.writeBits(static_cast<int>(predictor), 8);
        bs.writeBits(initialM, 16);
        bs.writeBits(adaptiveM ? 1 : 0, 1);

        // Initialize Golomb encoder
        Golomb golomb(initialM, Golomb::HandleSignApproach::ODD_EVEN_MAPPING);

        // Residual history for adaptive M
        std::vector<int> residualHistory;
        const size_t HISTORY_SIZE = 256;  // Window size for adaptation
        const size_t UPDATE_INTERVAL = 64;  // Update M every N pixels
        size_t pixelCount = 0;

        // Encode pixels row by row
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int actualPixel = image.at<uchar>(y, x);
                int prediction = 0;

                // First pixel - no prediction
                if (x == 0 && y == 0) {
                    bs.writeBits(actualPixel, 8);
                    continue;
                }

                // Predict pixel
                prediction = predictPixel(image, x, y);

                // Calculate residual
                int residual = actualPixel - prediction;

                // Encode residual
                golomb.encode(residual, bs);

                // Update adaptive M if enabled
                if (adaptiveM) {
                    updateResidualHistory(residualHistory, residual, HISTORY_SIZE);
                    pixelCount++;

                    if (pixelCount % UPDATE_INTERVAL == 0 && residualHistory.size() >= 32) {
                        int newM = calculateAdaptiveM(residualHistory);
                        golomb.setM(newM);
                    }
                }
            }
        }

        bs.close();

        // Calculate compressed size
        std::ifstream compFile(outputCompressedPath, std::ios::binary | std::ios::ate);
        compressedSize = compFile.tellg();
        compFile.close();

        std::cout << "Compression complete!" << std::endl;
        std::cout << "Original size: " << originalSize << " bytes" << std::endl;
        std::cout << "Compressed size: " << compressedSize << " bytes" << std::endl;
        std::cout << "Compression ratio: " << getCompressionRatio() << ":1" << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Encoding error: " << e.what() << std::endl;
        return false;
    }
}

bool LosslessImage::decode(const std::string& inputCompressedPath, 
                          const std::string& outputImagePath) {
    try {
        // Open compressed file
        BitStream bs(inputCompressedPath, std::ios::in);

        // Read header
        int width = bs.readBits(32);
        int height = bs.readBits(32);
        PredictorType decoderPredictor = static_cast<PredictorType>(bs.readBits(8));
        int decoderM = bs.readBits(16);
        bool decoderAdaptive = bs.readBits(1) == 1;

        std::cout << "Decoding image: " << width << "x" << height << std::endl;

        // Create output image
        cv::Mat image(height, width, CV_8UC1);

        // Initialize Golomb decoder
        Golomb golomb(decoderM, Golomb::HandleSignApproach::ODD_EVEN_MAPPING);

        // Residual history for adaptive M
        std::vector<int> residualHistory;
        const size_t HISTORY_SIZE = 256;
        const size_t UPDATE_INTERVAL = 64;
        size_t pixelCount = 0;

        // Decode pixels row by row
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int actualPixel = 0;

                // First pixel - no prediction
                if (x == 0 && y == 0) {
                    actualPixel = bs.readBits(8);
                    image.at<uchar>(y, x) = actualPixel;
                    continue;
                }

                // Predict pixel using same predictor as encoder
                int prediction = predictPixel(image, x, y);

                // Decode residual
                int residual = golomb.decode(bs);

                // Reconstruct pixel
                actualPixel = clipValue(prediction + residual);
                image.at<uchar>(y, x) = actualPixel;

                // Update adaptive M if enabled (must match encoder)
                if (decoderAdaptive) {
                    updateResidualHistory(residualHistory, residual, HISTORY_SIZE);
                    pixelCount++;

                    if (pixelCount % UPDATE_INTERVAL == 0 && residualHistory.size() >= 32) {
                        int newM = calculateAdaptiveM(residualHistory);
                        golomb.setM(newM);
                    }
                }
            }
        }

        bs.close();

        // Save decoded image
        cv::imwrite(outputImagePath, image);

        std::cout << "Decoding complete!" << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Decoding error: " << e.what() << std::endl;
        return false;
    }
}

double LosslessImage::getCompressionRatio() const {
    if (compressedSize == 0) return 0.0;
    return static_cast<double>(originalSize) / static_cast<double>(compressedSize);
}

size_t LosslessImage::getOriginalSize() const {
    return originalSize;
}

size_t LosslessImage::getCompressedSize() const {
    return compressedSize;
}

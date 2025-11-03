#ifndef LOSSLESS_IMAGE_HPP
#define LOSSLESS_IMAGE_HPP

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "bitstream.hpp"
#include "golomb.hpp"

enum class PredictorType {
    NONE = 0,
    LEFT = 1,
    TOP = 2,
    AVERAGE = 3,
    JPEGLS = 4
};

class LosslessImage {
private:
    PredictorType predictor;
    int initialM;
    bool adaptiveM;
    size_t originalSize;
    size_t compressedSize;

    int clipValue(int value) const;
    int predictPixel(const cv::Mat& image, int x, int y) const;
    int calculateAdaptiveM(const std::vector<int>& recentResiduals) const;
    void updateResidualHistory(std::vector<int>& history, int residual, size_t maxSize) const;
    int selectContextM(int gradientH, int gradientV) const;

public:
    LosslessImage(PredictorType predictor, int initialM = 16, bool adaptiveM = false);
    ~LosslessImage();

    bool encode(const std::string& inputImagePath, const std::string& outputCompressedPath);
    bool decode(const std::string& inputCompressedPath, const std::string& outputImagePath);

    double getCompressionRatio() const;
    size_t getOriginalSize() const;
    size_t getCompressedSize() const;
};

#endif // LOSSLESS_IMAGE_HPP

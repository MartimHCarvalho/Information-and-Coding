#include "image_codec.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

// Simple PGM (Portable GrayMap) file reader
bool readPGM(const std::string& filename, std::vector<uint8_t>& image, 
             int& width, int& height) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return false;
    }
    
    std::string magic;
    file >> magic;
    
    if (magic != "P5" && magic != "P2") {
        std::cerr << "Not a valid PGM file (expected P5 or P2)" << std::endl;
        return false;
    }
    
    // Skip comments
    char c;
    file.get(c);
    while (file.peek() == '#') {
        file.ignore(256, '\n');
    }
    
    file >> width >> height;
    int maxVal;
    file >> maxVal;
    
    if (maxVal != 255) {
        std::cerr << "Only 8-bit grayscale images supported" << std::endl;
        return false;
    }
    
    file.get(c); // Skip single whitespace after maxVal
    
    image.resize(width * height);
    
    if (magic == "P5") {
        // Binary format
        file.read(reinterpret_cast<char*>(image.data()), width * height);
    } else {
        // ASCII format
        for (int i = 0; i < width * height; i++) {
            int val;
            file >> val;
            image[i] = static_cast<uint8_t>(val);
        }
    }
    
    return true;
}

// Simple PGM file writer
bool writePGM(const std::string& filename, const std::vector<uint8_t>& image,
              int width, int height) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot create file: " << filename << std::endl;
        return false;
    }
    
    file << "P5\n" << width << " " << height << "\n255\n";
    file.write(reinterpret_cast<const char*>(image.data()), width * height);
    
    return true;
}

// Create a test grayscale image
std::vector<uint8_t> createTestImage(int width, int height) {
    std::vector<uint8_t> image(width * height);
    
    // Create a gradient with some patterns
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int val = (x * 255 / width + y * 255 / height) / 2;
            
            // Add some structure
            if ((x / 20 + y / 20) % 2 == 0) {
                val = (val + 50) % 256;
            }
            
            image[y * width + x] = static_cast<uint8_t>(val);
        }
    }
    
    return image;
}

void testCodec(const std::string& testName, 
               ImageCodec::PredictorType predictor,
               const std::vector<uint8_t>& image,
               int width, int height) {
    std::cout << "\n=== Testing " << testName << " ===" << std::endl;
    
    ImageCodec codec(predictor, 0, true); // Auto m, adaptive
    
    std::string compressedFile = "test_" + testName + ".golomb";
    std::string decodedFile = "test_" + testName + "_decoded.pgm";
    
    // Encode
    if (!codec.encode(image, width, height, compressedFile)) {
        std::cerr << "Encoding failed!" << std::endl;
        return;
    }
    
    auto stats = codec.getLastStats();
    std::cout << "\nCompression Statistics:" << std::endl;
    std::cout << "  Predictor: " << testName << std::endl;
    std::cout << "  Compression Ratio: " << stats.compressionRatio << ":1" << std::endl;
    std::cout << "  Bits per Pixel: " << stats.bitsPerPixel << std::endl;
    std::cout << "  Optimal m: " << stats.optimalM << std::endl;
    
    // Decode
    std::vector<uint8_t> decoded;
    int decWidth, decHeight;
    
    if (!codec.decode(compressedFile, decoded, decWidth, decHeight)) {
        std::cerr << "Decoding failed!" << std::endl;
        return;
    }
    
    // Verify
    if (decWidth != width || decHeight != height) {
        std::cerr << "Size mismatch!" << std::endl;
        return;
    }
    
    bool identical = true;
    for (size_t i = 0; i < image.size(); i++) {
        if (image[i] != decoded[i]) {
            identical = false;
            std::cerr << "Pixel mismatch at index " << i << ": "
                     << (int)image[i] << " vs " << (int)decoded[i] << std::endl;
            break;
        }
    }
    
    if (identical) {
        std::cout << "✓ Lossless compression verified!" << std::endl;
    } else {
        std::cerr << "✗ Decoded image differs from original!" << std::endl;
    }
    
    // Save decoded image
    writePGM(decodedFile, decoded, decWidth, decHeight);
}

int main(int argc, char* argv[]) {
    std::cout << "Lossless Image Codec - Golomb Coding Test" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    std::vector<uint8_t> image;
    int width, height;
    
    if (argc > 1) {
        // Load from PGM file
        std::cout << "\nLoading image from: " << argv[1] << std::endl;
        if (!readPGM(argv[1], image, width, height)) {
            return 1;
        }
        std::cout << "Image size: " << width << "x" << height << std::endl;
    } else {
        // Create test image
        std::cout << "\nNo input file provided. Creating test image..." << std::endl;
        width = 256;
        height = 256;
        image = createTestImage(width, height);
        writePGM("test_original.pgm", image, width, height);
        std::cout << "Test image saved as: test_original.pgm" << std::endl;
    }
    
    // Test different predictors
    testCodec("JPEGLS", ImageCodec::PredictorType::JPEGLS, 
              image, width, height);
    
    testCodec("Paeth", ImageCodec::PredictorType::PAETH, 
              image, width, height);
    
    testCodec("Average", ImageCodec::PredictorType::AVERAGE, 
              image, width, height);
    
    testCodec("Adaptive", ImageCodec::PredictorType::ADAPTIVE, 
              image, width, height);
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    std::cout << "\nBest predictor: JPEGLS or Adaptive (typically)" << std::endl;
    std::cout << "The codec automatically selects optimal m based on residual statistics." << std::endl;
    
    return 0;
}
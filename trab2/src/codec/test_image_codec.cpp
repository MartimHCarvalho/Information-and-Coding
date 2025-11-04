#include "../../includes/codec/image_codec.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <cmath>

// ==================== Image Testing Utilities ====================

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
        std::cerr << "Not a valid PGM file" << std::endl;
        return false;
    }
    
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
    
    file.get(c);
    image.resize(width * height);
    
    if (magic == "P5") {
        file.read(reinterpret_cast<char*>(image.data()), width * height);
    } else {
        for (int i = 0; i < width * height; i++) {
            int val;
            file >> val;
            image[i] = static_cast<uint8_t>(val);
        }
    }
    
    return true;
}

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

std::vector<uint8_t> createTestImage(int width, int height) {
    std::vector<uint8_t> image(width * height);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int val = (x * 255 / width + y * 255 / height) / 2;
            
            // Add structure
            if ((x / 20 + y / 20) % 2 == 0) {
                val = (val + 50) % 256;
            }
            
            // Add noise
            val += (x * y) % 10 - 5;
            val = std::max(0, std::min(255, val));
            
            image[y * width + x] = static_cast<uint8_t>(val);
        }
    }
    
    return image;
}

// ==================== Testing Functions ====================

void printHeader(const std::string& title) {
    std::cout << "\n╔" << std::string(58, '═') << "╗" << std::endl;
    std::cout << "║ " << std::setw(56) << std::left << title << " ║" << std::endl;
    std::cout << "╚" << std::string(58, '═') << "╝" << std::endl;
}

void testImageCodec(const std::string& testName, 
                    ImageCodec::PredictorType predictor,
                    const std::vector<uint8_t>& image,
                    int width, int height) {
    std::cout << "\n" << std::string(60, '-') << std::endl;
    std::cout << "Image Test: " << testName << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    ImageCodec codec(predictor, 0, true);
    
    std::string compressedFile = "test_img_" + testName + ".golomb";
    std::string decodedFile = "test_img_" + testName + "_decoded.pgm";
    
    // Encode
    auto startEncode = std::chrono::high_resolution_clock::now();
    if (!codec.encode(image, width, height, compressedFile)) {
        std::cerr << "✗ Encoding failed!" << std::endl;
        return;
    }
    auto endEncode = std::chrono::high_resolution_clock::now();
    auto encodeTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        endEncode - startEncode).count();
    
    auto stats = codec.getLastStats();
    std::cout << "\n📊 Results:" << std::endl;
    std::cout << "  ├─ Compression ratio:   " << std::fixed << std::setprecision(2) 
              << stats.compressionRatio << ":1" << std::endl;
    std::cout << "  ├─ Bits per pixel:      " << std::setprecision(3) 
              << stats.bitsPerPixel << " (orig: 8.0)" << std::endl;
    std::cout << "  ├─ Space savings:       " << std::setprecision(1) 
              << (100.0 * (1.0 - 1.0/stats.compressionRatio)) << "%" << std::endl;
    std::cout << "  ├─ Optimal m:           " << stats.optimalM << std::endl;
    std::cout << "  └─ Encoding time:       " << encodeTime << " ms" << std::endl;
    
    // Decode
    auto startDecode = std::chrono::high_resolution_clock::now();
    std::vector<uint8_t> decoded;
    int decWidth, decHeight;
    
    if (!codec.decode(compressedFile, decoded, decWidth, decHeight)) {
        std::cerr << "✗ Decoding failed!" << std::endl;
        return;
    }
    auto endDecode = std::chrono::high_resolution_clock::now();
    auto decodeTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        endDecode - startDecode).count();
    
    std::cout << "\n⚙️  Decoding time:        " << decodeTime << " ms" << std::endl;
    
    // Verify
    bool identical = (decWidth == width && decHeight == height && 
                     image.size() == decoded.size());
    
    if (identical) {
        for (size_t i = 0; i < image.size(); i++) {
            if (image[i] != decoded[i]) {
                identical = false;
                break;
            }
        }
    }
    
    if (identical) {
        std::cout << "\n✓ Perfect reconstruction verified!" << std::endl;
    } else {
        std::cerr << "\n✗ Decoded image differs from original!" << std::endl;
    }
    
    writePGM(decodedFile, decoded, decWidth, decHeight);
}

// ==================== Main Test Program ====================

int main(int argc, char* argv[]) {
    printHeader("Image Codec Test Suite - Golomb Coding");
    
    std::cout << "\nThis test suite evaluates the image codec with different predictors." << std::endl;
    std::cout << "Usage: " << argv[0] << " [image.pgm]" << std::endl;
    
    std::vector<uint8_t> image;
    int width, height;
    
    // Load or create test image
    if (argc > 1) {
        std::cout << "\n📁 Loading image from: " << argv[1] << std::endl;
        if (!readPGM(argv[1], image, width, height)) {
            std::cerr << "Failed to load image, creating test image instead..." << std::endl;
        } else {
            std::cout << "   Size: " << width << "x" << height << " pixels" << std::endl;
        }
    }
    
    if (image.empty()) {
        std::cout << "\n⚠️  Creating test image (256x256)..." << std::endl;
        width = 256;
        height = 256;
        image = createTestImage(width, height);
        writePGM("test_original.pgm", image, width, height);
        std::cout << "   Saved as: test_original.pgm" << std::endl;
    }
    
    // Test different predictors
    testImageCodec("JPEGLS", ImageCodec::PredictorType::JPEGLS, 
                   image, width, height);
    
    testImageCodec("Paeth", ImageCodec::PredictorType::PAETH, 
                   image, width, height);
    
    testImageCodec("Average", ImageCodec::PredictorType::AVERAGE, 
                   image, width, height);
    
    testImageCodec("Left", ImageCodec::PredictorType::LEFT, 
                   image, width, height);
    
    testImageCodec("Adaptive", ImageCodec::PredictorType::ADAPTIVE, 
                   image, width, height);
    
    // Summary
    printHeader("TEST SUITE COMPLETE");
    
    std::cout << "\n💡 Best Practices Summary:" << std::endl;
    std::cout << "\n  Predictors:" << std::endl;
    std::cout << "    • JPEGLS predictor - best for natural images" << std::endl;
    std::cout << "    • Paeth predictor - good for computer-generated images" << std::endl;
    std::cout << "    • Adaptive predictor - auto-selects best per row" << std::endl;
    std::cout << "    • Average predictor - balanced performance" << std::endl;
    std::cout << "    • Left predictor - simple, fast, lower compression" << std::endl;
    
    std::cout << "\n  Optimization:" << std::endl;
    std::cout << "    • Adaptive m provides optimal compression" << std::endl;
    std::cout << "    • Golomb coding optimal for geometric distributions" << std::endl;
    std::cout << "    • All codecs are perfectly lossless" << std::endl;
    
    std::cout << "\n  Output Files:" << std::endl;
    std::cout << "    • test_img_*.golomb - compressed files" << std::endl;
    std::cout << "    • test_img_*_decoded.pgm - reconstructed images" << std::endl;
    std::cout << std::endl;
    
    return 0;
}
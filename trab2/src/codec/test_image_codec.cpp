#include "../../includes/codec/image_codec.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <map>
#include <sstream>
#include <algorithm> // ADD THIS LINE for std::max_element

// ==================== Color Codes ====================
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define BOLD "\033[1m"

// ==================== Image Testing Utilities ====================

bool readPGM(const std::string &filename, std::vector<uint8_t> &image,
             int &width, int &height)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        std::cerr << RED << "Cannot open file: " << filename << RESET << std::endl;
        return false;
    }

    std::string magic;
    file >> magic;

    if (magic != "P5" && magic != "P2")
    {
        std::cerr << RED << "Not a valid PGM file" << RESET << std::endl;
        return false;
    }

    char c;
    file.get(c);
    while (file.peek() == '#')
    {
        file.ignore(256, '\n');
    }

    file >> width >> height;
    int maxVal;
    file >> maxVal;

    if (maxVal != 255)
    {
        std::cerr << RED << "Only 8-bit grayscale images supported" << RESET << std::endl;
        return false;
    }

    file.get(c);
    image.resize(width * height);

    if (magic == "P5")
    {
        file.read(reinterpret_cast<char *>(image.data()), width * height);
    }
    else
    {
        for (int i = 0; i < width * height; i++)
        {
            int val;
            file >> val;
            image[i] = static_cast<uint8_t>(val);
        }
    }

    return true;
}

bool writePGM(const std::string &filename, const std::vector<uint8_t> &image,
              int width, int height)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file)
    {
        std::cerr << RED << "Cannot create file: " << filename << RESET << std::endl;
        return false;
    }

    file << "P5\n"
         << width << " " << height << "\n255\n";
    file.write(reinterpret_cast<const char *>(image.data()), width * height);

    return true;
}

std::vector<uint8_t> createTestImage(int width, int height, const std::string &type = "gradient")
{
    std::vector<uint8_t> image(width * height);

    if (type == "gradient")
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int val = (x * 255 / width + y * 255 / height) / 2;

                // Add structure
                if ((x / 20 + y / 20) % 2 == 0)
                {
                    val = (val + 50) % 256;
                }

                // Add noise
                val += (x * y) % 10 - 5;
                val = std::max(0, std::min(255, val));

                image[y * width + x] = static_cast<uint8_t>(val);
            }
        }
    }
    else if (type == "checkerboard")
    {
        int blockSize = 16;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                bool isWhite = ((x / blockSize) + (y / blockSize)) % 2 == 0;
                image[y * width + x] = isWhite ? 255 : 0;
            }
        }
    }
    else if (type == "circles")
    {
        int cx = width / 2;
        int cy = height / 2;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int dx = x - cx;
                int dy = y - cy;
                int dist = std::sqrt(dx * dx + dy * dy);
                image[y * width + x] = static_cast<uint8_t>((dist * 255) / (width / 2));
            }
        }
    }

    return image;
}

void printImageInfo(const std::string &filename, int width, int height)
{
    std::cout << CYAN << "🖼️  Image Information:" << RESET << std::endl;
    std::cout << "  ├─ File: " << filename << std::endl;
    std::cout << "  ├─ Dimensions: " << width << "x" << height << " pixels" << std::endl;
    std::cout << "  ├─ Total pixels: " << width * height << std::endl;
    std::cout << "  └─ Size: " << std::fixed << std::setprecision(2)
              << (width * height) / 1024.0 << " KB" << std::endl;
}

// ==================== Image Statistics ====================

struct ImageStatistics
{
    double mean;
    double stdDev;
    uint8_t minValue;
    uint8_t maxValue;
    double entropy;
};

ImageStatistics analyzeImage(const std::vector<uint8_t> &image)
{
    ImageStatistics stats = {0, 0, 255, 0, 0};

    if (image.empty())
        return stats;

    // Calculate mean, min, max
    double sum = 0;
    std::map<uint8_t, int> histogram;

    for (uint8_t pixel : image)
    {
        sum += pixel;
        stats.minValue = std::min(stats.minValue, pixel);
        stats.maxValue = std::max(stats.maxValue, pixel);
        histogram[pixel]++;
    }

    stats.mean = sum / image.size();

    // Calculate standard deviation
    double sumSquaredDiff = 0;
    for (uint8_t pixel : image)
    {
        double diff = pixel - stats.mean;
        sumSquaredDiff += diff * diff;
    }
    stats.stdDev = std::sqrt(sumSquaredDiff / image.size());

    // Calculate entropy
    stats.entropy = 0;
    for (const auto &pair : histogram)
    {
        double p = static_cast<double>(pair.second) / image.size();
        if (p > 0)
        {
            stats.entropy -= p * std::log2(p);
        }
    }

    return stats;
}

void printImageStatistics(const ImageStatistics &stats)
{
    std::cout << YELLOW << "📊 Image Statistics:" << RESET << std::endl;
    std::cout << "  ├─ Mean: " << std::fixed << std::setprecision(2) << stats.mean << std::endl;
    std::cout << "  ├─ Std Dev: " << stats.stdDev << std::endl;
    std::cout << "  ├─ Range: [" << (int)stats.minValue << ", " << (int)stats.maxValue << "]" << std::endl;
    std::cout << "  └─ Entropy: " << std::setprecision(3) << stats.entropy << " bits/pixel" << std::endl;
}

// ==================== Testing Framework ====================

class ImageCodecTester
{
private:
    struct TestResult
    {
        std::string testName;
        std::string predictor;
        double compressionRatio;
        double bitsPerPixel;
        double spaceSavings;
        int optimalM;
        long long encodeTimeMs;
        long long decodeTimeMs;
        bool reconstructionPerfect;
        size_t originalSize;
        size_t compressedSize;
    };

    std::vector<TestResult> results;

    std::string predictorToString(ImageCodec::PredictorType pred) const
    {
        switch (pred)
        {
        case ImageCodec::PredictorType::NONE:
            return "NONE";
        case ImageCodec::PredictorType::LEFT:
            return "LEFT";
        case ImageCodec::PredictorType::TOP:
            return "TOP";
        case ImageCodec::PredictorType::AVERAGE:
            return "AVERAGE";
        case ImageCodec::PredictorType::PAETH:
            return "PAETH";
        case ImageCodec::PredictorType::JPEGLS:
            return "JPEGLS";
        case ImageCodec::PredictorType::ADAPTIVE:
            return "ADAPTIVE";
        default:
            return "UNKNOWN";
        }
    }

public:
    void runTest(const std::string &testName,
                 ImageCodec::PredictorType predictor,
                 const std::vector<uint8_t> &image,
                 int width, int height)
    {

        printTestHeader(testName);

        ImageCodec codec(predictor, 0, true);

        std::string compressedFile = "test_img_" + testName + ".golomb";
        std::string decodedFile = "test_img_" + testName + "_decoded.pgm";

        // Encode
        auto startEncode = std::chrono::high_resolution_clock::now();
        if (!codec.encode(image, width, height, compressedFile))
        {
            std::cerr << RED << "✗ Encoding failed!" << RESET << std::endl;
            return;
        }
        auto endEncode = std::chrono::high_resolution_clock::now();
        auto encodeTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                              endEncode - startEncode)
                              .count();

        auto stats = codec.getLastStats();

        printCompressionResults(stats, encodeTime);

        // Decode
        auto startDecode = std::chrono::high_resolution_clock::now();
        std::vector<uint8_t> decoded;
        int decWidth, decHeight;

        if (!codec.decode(compressedFile, decoded, decWidth, decHeight))
        {
            std::cerr << RED << "✗ Decoding failed!" << RESET << std::endl;
            return;
        }
        auto endDecode = std::chrono::high_resolution_clock::now();
        auto decodeTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                              endDecode - startDecode)
                              .count();

        std::cout << "\n⚙️  Decoding time:        " << decodeTime << " ms" << std::endl;

        // Verify
        bool identical = verifyReconstruction(image, decoded, width, height, decWidth, decHeight);

        if (identical)
        {
            std::cout << GREEN << "\n✓ Perfect reconstruction verified!" << RESET << std::endl;
        }
        else
        {
            std::cerr << RED << "\n✗ Decoded image differs from original!" << RESET << std::endl;
        }

        writePGM(decodedFile, decoded, decWidth, decHeight);

        // Store results
        TestResult result;
        result.testName = testName;
        result.predictor = predictorToString(predictor);
        result.compressionRatio = stats.compressionRatio;
        result.bitsPerPixel = stats.bitsPerPixel;
        result.spaceSavings = 100.0 * (1.0 - 1.0 / stats.compressionRatio);
        result.optimalM = stats.optimalM;
        result.encodeTimeMs = encodeTime;
        result.decodeTimeMs = decodeTime;
        result.reconstructionPerfect = identical;
        result.originalSize = stats.originalSize;
        result.compressedSize = stats.compressedSize;
        results.push_back(result);
    }

    void printSummary()
    {
        if (results.empty())
            return;

        std::cout << "\n"
                  << BOLD << CYAN;
        std::cout << "╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "║              COMPRESSION SUMMARY                         ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════╝";
        std::cout << RESET << std::endl;

        // Find best configuration
        auto best = std::max_element(results.begin(), results.end(),
                                     [](const TestResult &a, const TestResult &b)
                                     {
                                         return a.compressionRatio < b.compressionRatio;
                                     });

        std::cout << "\n"
                  << GREEN << "🏆 Best Configuration:" << RESET << std::endl;
        std::cout << "  Test: " << BOLD << best->testName << RESET << std::endl;
        std::cout << "  Compression Ratio: " << BOLD << std::fixed << std::setprecision(2)
                  << best->compressionRatio << ":1" << RESET << std::endl;
        std::cout << "  Bits per Pixel: " << std::setprecision(3)
                  << best->bitsPerPixel << std::endl;
        std::cout << "  Space Savings: " << std::setprecision(1)
                  << best->spaceSavings << "%" << std::endl;

        // Comparison table
        std::cout << "\n"
                  << YELLOW << "📊 All Results:" << RESET << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        std::cout << std::left << std::setw(15) << "Test Name"
                  << std::setw(12) << "Ratio"
                  << std::setw(12) << "Bits/Pix"
                  << std::setw(12) << "Enc (ms)"
                  << std::setw(12) << "Dec (ms)"
                  << "Status" << std::endl;
        std::cout << std::string(80, '-') << std::endl;

        for (const auto &r : results)
        {
            std::cout << std::left << std::setw(15) << r.testName
                      << std::setw(12) << std::fixed << std::setprecision(2) << r.compressionRatio
                      << std::setw(12) << std::setprecision(3) << r.bitsPerPixel
                      << std::setw(12) << r.encodeTimeMs
                      << std::setw(12) << r.decodeTimeMs;

            if (r.reconstructionPerfect)
            {
                std::cout << GREEN << "✓ OK" << RESET;
            }
            else
            {
                std::cout << RED << "✗ FAIL" << RESET;
            }
            std::cout << std::endl;
        }
        std::cout << std::string(80, '-') << std::endl;

        // Statistics
        double avgRatio = 0, avgBits = 0;
        for (const auto &r : results)
        {
            avgRatio += r.compressionRatio;
            avgBits += r.bitsPerPixel;
        }
        avgRatio /= results.size();
        avgBits /= results.size();

        std::cout << "\n"
                  << CYAN << "📈 Average Statistics:" << RESET << std::endl;
        std::cout << "  Compression Ratio: " << std::fixed << std::setprecision(2)
                  << avgRatio << ":1" << std::endl;
        std::cout << "  Bits per Pixel: " << std::setprecision(3) << avgBits << std::endl;
    }

    void exportResultsJSON(const std::string &filename = "image_codec_results.json")
    {
        std::ofstream json(filename);
        if (!json)
        {
            std::cerr << RED << "✗ Cannot create JSON file: " << filename << RESET << std::endl;
            return;
        }

        json << "[\n";
        for (size_t i = 0; i < results.size(); i++)
        {
            const auto &r = results[i];
            json << "  {\n"
                 << "    \"test_name\": \"" << r.testName << "\",\n"
                 << "    \"predictor\": \"" << r.predictor << "\",\n"
                 << "    \"compression_ratio\": " << std::fixed << std::setprecision(6) << r.compressionRatio << ",\n"
                 << "    \"bits_per_pixel\": " << r.bitsPerPixel << ",\n"
                 << "    \"space_savings\": " << r.spaceSavings << ",\n"
                 << "    \"optimal_m\": " << r.optimalM << ",\n"
                 << "    \"encode_time\": " << r.encodeTimeMs << ",\n"
                 << "    \"decode_time\": " << r.decodeTimeMs << ",\n"
                 << "    \"reconstruction_perfect\": " << (r.reconstructionPerfect ? "true" : "false") << ",\n"
                 << "    \"original_size\": " << r.originalSize << ",\n"
                 << "    \"compressed_size\": " << r.compressedSize << "\n"
                 << "  }" << (i < results.size() - 1 ? "," : "") << "\n";
        }
        json << "]\n";
        json.close();

        std::cout << GREEN << "\n✓ Results exported to: " << filename << RESET << std::endl;
    }

private:
    void printTestHeader(const std::string &testName)
    {
        std::cout << "\n"
                  << std::string(70, '-') << std::endl;
        std::cout << BOLD << BLUE << "🖼️  Image Test: " << testName << RESET << std::endl;
        std::cout << std::string(70, '-') << std::endl;
    }

    void printCompressionResults(const ImageCodec::CompressionStats &stats, long long encodeTime)
    {
        std::cout << "\n"
                  << MAGENTA << "📊 Compression Results:" << RESET << std::endl;
        std::cout << "  ├─ Compression ratio:   " << std::fixed << std::setprecision(2)
                  << stats.compressionRatio << ":1" << std::endl;
        std::cout << "  ├─ Bits per pixel:      " << std::setprecision(3)
                  << stats.bitsPerPixel << " (original: 8.0)" << std::endl;
        std::cout << "  ├─ Space savings:       " << std::setprecision(1)
                  << (100.0 * (1.0 - 1.0 / stats.compressionRatio)) << "%" << std::endl;
        std::cout << "  ├─ Original size:       " << stats.originalSize << " bytes" << std::endl;
        std::cout << "  ├─ Compressed size:     " << stats.compressedSize << " bytes" << std::endl;
        std::cout << "  ├─ Optimal m:           " << stats.optimalM << std::endl;
        std::cout << "  └─ Encoding time:       " << encodeTime << " ms" << std::endl;
    }

    bool verifyReconstruction(const std::vector<uint8_t> &original,
                              const std::vector<uint8_t> &decoded,
                              int width, int height,
                              int decWidth, int decHeight)
    {
        if (width != decWidth || height != decHeight)
        {
            std::cerr << YELLOW << "  Dimension mismatch: " << width << "x" << height
                      << " vs " << decWidth << "x" << decHeight << RESET << std::endl;
            return false;
        }

        if (original.size() != decoded.size())
        {
            std::cerr << YELLOW << "  Size mismatch: " << original.size()
                      << " vs " << decoded.size() << RESET << std::endl;
            return false;
        }

        for (size_t i = 0; i < original.size(); i++)
        {
            if (original[i] != decoded[i])
            {
                std::cerr << YELLOW << "  Difference at pixel " << i << ": "
                          << (int)original[i] << " vs " << (int)decoded[i] << RESET << std::endl;
                return false;
            }
        }
        return true;
    }
};

// ==================== Main Program ====================

void printBanner()
{
    std::cout << BOLD << CYAN;
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║           Image Codec Test Suite - Golomb Coding        ║\n";
    std::cout << "║                   Lossless Image Compression             ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    std::cout << RESET << std::endl;
}

void printUsage(const char *programName)
{
    std::cout << "Usage: " << programName << " [image.pgm] [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  No arguments     - Run with synthetic test image (256x256)\n";
    std::cout << "  image.pgm        - Test with your PGM file\n";
    std::cout << "  -h, --help       - Show this help message\n";
    std::cout << "  -v, --verbose    - Show detailed analysis\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << "\n";
    std::cout << "  " << programName << " myimage.pgm\n";
    std::cout << "  " << programName << " /path/to/image.pgm -v\n";
}

int main(int argc, char *argv[])
{
    printBanner();

    // Parse arguments
    bool verbose = false;
    std::string inputFile = "";

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help")
        {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "-v" || arg == "--verbose")
        {
            verbose = true;
        }
        else if (arg[0] != '-')
        {
            inputFile = arg;
        }
    }

    // Load or generate image
    std::vector<uint8_t> image;
    int width, height;
    std::string imageSource;

    if (!inputFile.empty())
    {
        std::cout << CYAN << "📂 Loading image from: " << inputFile << RESET << std::endl;
        if (!readPGM(inputFile, image, width, height))
        {
            std::cerr << RED << "Failed to load image file" << RESET << std::endl;
            std::cout << YELLOW << "\n💡 Creating synthetic image instead..." << RESET << std::endl;
            inputFile = "";
        }
        else
        {
            printImageInfo(inputFile, width, height);
            imageSource = inputFile;
        }
    }

    if (inputFile.empty())
    {
        std::cout << YELLOW << "\n⚠️  No image file provided. Creating synthetic test image..." << RESET << std::endl;
        width = 256;
        height = 256;

        std::cout << "  ├─ Type: Gradient with structure" << std::endl;
        std::cout << "  ├─ Dimensions: " << width << "x" << height << " pixels" << std::endl;
        std::cout << "  └─ Color: 8-bit grayscale" << std::endl;

        image = createTestImage(width, height, "gradient");

        std::string synthFile = "test_image_original.pgm";
        writePGM(synthFile, image, width, height);
        std::cout << GREEN << "\n✓ Synthetic image saved as: " << synthFile << RESET << std::endl;
        imageSource = synthFile;
    }

    // Analyze image if verbose
    if (verbose)
    {
        std::cout << "\n"
                  << BOLD << MAGENTA << "📊 Image Analysis:" << RESET << std::endl;
        auto stats = analyzeImage(image);
        printImageStatistics(stats);
    }

    // Initialize tester
    ImageCodecTester tester;

    // Run tests
    std::cout << "\n"
              << BOLD << GREEN;
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║              STARTING COMPRESSION TESTS                  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝";
    std::cout << RESET << std::endl;

    tester.runTest("JPEGLS",
                   ImageCodec::PredictorType::JPEGLS,
                   image, width, height);

    tester.runTest("Paeth",
                   ImageCodec::PredictorType::PAETH,
                   image, width, height);

    tester.runTest("Average",
                   ImageCodec::PredictorType::AVERAGE,
                   image, width, height);

    tester.runTest("Left",
                   ImageCodec::PredictorType::LEFT,
                   image, width, height);

    tester.runTest("Adaptive",
                   ImageCodec::PredictorType::ADAPTIVE,
                   image, width, height);

    // Print summary
    tester.printSummary();

    // Export to JSON
    tester.exportResultsJSON("image_codec_results.json");

    // Final recommendations
    std::cout << "\n"
              << BOLD << CYAN;
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║              RECOMMENDATIONS                             ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝";
    std::cout << RESET << std::endl;

    std::cout << "\n"
              << YELLOW << "💡 Best Practices:" << RESET << std::endl;
    std::cout << "\n  " << BOLD << "Predictors:" << RESET << std::endl;
    std::cout << "    • JPEGLS - Best for natural images with smooth gradients" << std::endl;
    std::cout << "    • Paeth - Good for computer-generated images" << std::endl;
    std::cout << "    • Adaptive - Auto-selects best predictor per row" << std::endl;
    std::cout << "    • Average - Balanced performance" << std::endl;
    std::cout << "    • Left - Simple, fast, lower compression" << std::endl;

    std::cout << "\n  " << BOLD << "General:" << RESET << std::endl;
    std::cout << "    • All codecs are perfectly lossless" << std::endl;
    std::cout << "    • Adaptive m parameter optimizes per block" << std::endl;
    std::cout << "    • Compression varies with image characteristics" << std::endl;

    std::cout << "\n  " << BOLD << "Output Files:" << RESET << std::endl;
    std::cout << "    • test_img_*.golomb - Compressed image files" << std::endl;
    std::cout << "    • test_img_*_decoded.pgm - Reconstructed images" << std::endl;
    std::cout << "    • image_codec_results.json - Machine-readable results" << std::endl;
    std::cout << "    • Analyze with: python visualizerImage.py image_codec_results.json" << std::endl;

    std::cout << "\n  " << BOLD << "Expected Performance:" << RESET << std::endl;
    std::cout << "    • Natural images:  1.5-2.5:1 compression (3-5 bits/pixel)" << std::endl;
    std::cout << "    • Smooth gradients: 2.0-3.0:1 compression (2.7-4 bits/pixel)" << std::endl;
    std::cout << "    • Text/diagrams:   1.8-2.5:1 compression (3-4.5 bits/pixel)" << std::endl;
    std::cout << "    • Random noise:    1.0-1.2:1 compression (6.7-8 bits/pixel)" << std::endl;

    std::cout << "\n"
              << BOLD << GREEN;
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║              TEST SUITE COMPLETE ✓                       ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝";
    std::cout << RESET << "\n"
              << std::endl;

    return 0;
}
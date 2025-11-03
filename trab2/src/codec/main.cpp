#include "codec/losslessImage.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <encode|decode> <input> <output>\n";
        return 1;
    }

    std::string mode = argv[1];
    std::string input = argv[2];
    std::string output = argv[3];

    LosslessImage codec(PredictorType::JPEGLS, 16, true);

    if (mode == "encode") {
        if (codec.encode(input, output)) {
            std::cout << "Encoded successfully\n";
            std::cout << "Compression ratio: " << codec.getCompressionRatio() << "\n";
        } else {
            std::cerr << "Encoding failed\n";
            return 1;
        }
    } else if (mode == "decode") {
        if (codec.decode(input, output)) {
            std::cout << "Decoded successfully\n";
        } else {
            std::cerr << "Decoding failed\n";
            return 1;
        }
    } else {
        std::cerr << "Invalid mode. Use 'encode' or 'decode'\n";
        return 1;
    }

    return 0;
}
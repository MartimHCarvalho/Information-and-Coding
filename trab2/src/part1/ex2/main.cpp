#include "../includes/Image.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>

void invertColors(Image& img);
void flipHorizontal(Image& img);
void flipVertical(Image& img);
Image rotateImage90(const Image& src, int times);
void adjustIntensity(Image& img, int value);

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " input.ppm output.ppm operation [value]\n";
        std::cout << "Operations:\n"
                  << "  negative\n"
                  << "  mirror_h\n"
                  << "  mirror_v\n"
                  << "  rotate <times_of_90_deg>\n"
                  << "  intensity <value>\n";
        return 1;
    }

    Image img;
    if (!img.load(argv[1])) return 1;

    std::string op = argv[3];

    if (op == "negative") invertColors(img);
    else if (op == "mirror_h") flipHorizontal(img);
    else if (op == "mirror_v") flipVertical(img);
    else if (op == "rotate") {
        if (argc < 5) {
            std::cerr << "Rotation requires an integer parameter.\n";
            return 1;
        }
        int times = std::atoi(argv[4]);
        img = rotateImage90(img, times);
    } 
    else if (op == "intensity") {
        if (argc < 5) {
            std::cerr << "Intensity requires an integer parameter.\n";
            return 1;
        }
        int val = std::atoi(argv[4]);
        adjustIntensity(img, val);
    } 
    else {
        std::cerr << "Unknown operation: " << op << "\n";
        return 1;
    }

    img.save(argv[2]);
    return 0;
}
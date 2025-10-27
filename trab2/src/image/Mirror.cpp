#include "../includes/image/Image.hpp"
#include <algorithm>

void flipHorizontal(Image& img) {
    int w = img.width, h = img.height, c = img.channels;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w / 2; ++x) {
            for (int ch = 0; ch < c; ++ch) {
                std::swap(
                    img.data[(y * w + x) * c + ch],
                    img.data[(y * w + (w - 1 - x)) * c + ch]
                );
            }
        }
    }
}

void flipVertical(Image& img) {
    int w = img.width, h = img.height, c = img.channels;
    for (int y = 0; y < h / 2; ++y) {
        for (int x = 0; x < w; ++x) {
            for (int ch = 0; ch < c; ++ch) {
                std::swap(
                    img.data[(y * w + x) * c + ch],
                    img.data[((h - 1 - y) * w + x) * c + ch]
                );
            }
        }
    }
}
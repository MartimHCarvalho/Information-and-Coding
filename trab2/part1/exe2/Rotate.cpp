#include "Image.h"

Image rotateImage90(const Image& src, int times) {
    times %= 4;
    if (times == 0) return src;

    Image rotated;
    rotated.channels = src.channels;
    rotated.max_val = src.max_val;

    int w = src.width;
    int h = src.height;
    int c = src.channels;

    if (times % 2 == 1) {
        rotated.width = h;
        rotated.height = w;
    } else {
        rotated.width = w;
        rotated.height = h;
    }

    rotated.data.resize(rotated.width * rotated.height * c);

    if (times == 1) { // 90 degrees
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                for (int ch = 0; ch < c; ++ch) {
                    rotated.data[(x * rotated.width + (rotated.width - 1 - y)) * c + ch] =
                        src.data[(y * w + x) * c + ch];
                }
            }
        }
    }
    return rotated;
}
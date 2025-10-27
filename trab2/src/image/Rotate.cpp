#include "../includes/image/Image.hpp"

Image rotateImage90(const Image& src, int times) {
    times = ((times % 4) + 4) % 4;  // avoid negatives

    if (times == 0)
        return src;

    Image rotated;
    rotated.channels = src.channels;
    rotated.max_val = src.max_val;
    int w = src.width;
    int h = src.height;
    int c = src.channels;

    // adjust rotated image dimensions
    rotated.width = (times % 2 == 1) ? h : w;
    rotated.height = (times % 2 == 1) ? w : h;
    rotated.data.resize(rotated.width * rotated.height * c);

    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            int newRow, newCol;
            switch (times) {
                case 1: // 90°
                    newRow = col;
                    newCol = rotated.height - 1 - row;
                    break;
                case 2: // 180°
                    newRow = rotated.height - 1 - row;
                    newCol = rotated.width - 1 - col;
                    break;
                case 3: // 270°
                    newRow = rotated.width - 1 - col;
                    newCol = row;
                    break;
                default:
                    newRow = row;
                    newCol = col;
            }

            for (int ch = 0; ch < c; ++ch) {
                rotated.data[(newRow * rotated.width + newCol) * c + ch] =
                    src.data[(row * w + col) * c + ch];
            }
        }
    }

    return rotated;
}
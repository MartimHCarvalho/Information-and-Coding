#include "../includes/Image.hpp"

void invertColors(Image& img) {
    for (size_t i = 0; i < img.size(); ++i)
        img.data[i] = img.max_val - img.data[i];
}
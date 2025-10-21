#include "Image.h"

void adjustIntensity(Image& img, int value) {
    for (size_t i = 0; i < img.size(); ++i) {
        int newVal = img.data[i] + value;
        if (newVal > img.max_val) newVal = img.max_val;
        else if (newVal < 0) newVal = 0;
        img.data[i] = static_cast<unsigned char>(newVal);
    }
}
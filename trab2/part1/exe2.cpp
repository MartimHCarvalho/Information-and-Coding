#include <cstdio>
#include <cstdlib>
#include <cstring>

struct Image {
    int width;
    int height;
    int max_val;
    unsigned char* data; // Pixel data (RGB or grayscale)
    int channels;        // Number of color channels (1 or 3)
};

// Read PPM (P6) or PGM (P5) image from file
Image* loadImage(const char* filename) {
    FILE* file = std::fopen(filename, "rb");
    if (!file) {
        std::perror("Unable to open file");
        return nullptr;
    }

    char format[3];
    std::fscanf(file, "%2s", format);

    if (std::strcmp(format, "P6") != 0 && std::strcmp(format, "P5") != 0) {
        std::fprintf(stderr, "Unsupported image format. Use P6 or P5.\n");
        std::fclose(file);
        return nullptr;
    }

    int width, height, max_val;
    std::fscanf(file, "%d %d %d%*c", &width, &height, &max_val);

    Image* img = static_cast<Image*>(std::malloc(sizeof(Image)));
    img->width = width;
    img->height = height;
    img->max_val = max_val;
    img->channels = (format[1] == '6') ? 3 : 1;
    img->data = static_cast<unsigned char*>(std::malloc(width * height * img->channels));

    std::fread(img->data, 1, width * height * img->channels, file);
    std::fclose(file);

    return img;
}

// Save image to PPM/PGM file
void saveImage(const char* filename, Image* img) {
    FILE* file = std::fopen(filename, "wb");
    if (!file) {
        std::perror("Unable to open file for writing");
        return;
    }

    if (img->channels == 3)
        std::fprintf(file, "P6\n%d %d\n%d\n", img->width, img->height, img->max_val);
    else
        std::fprintf(file, "P5\n%d %d\n%d\n", img->width, img->height, img->max_val);

    std::fwrite(img->data, 1, img->width * img->height * img->channels, file);
    std::fclose(file);
}

void invertColors(Image* img) {
    int total = img->width * img->height * img->channels;
    for (int i = 0; i < total; ++i) {
        img->data[i] = img->max_val - img->data[i];
    }
}

void flipHorizontal(Image* img) {
    int w = img->width;
    int h = img->height;
    int c = img->channels;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w / 2; ++x) {
            for (int ch = 0; ch < c; ++ch) {
                int leftIdx = (y * w + x) * c + ch;
                int rightIdx = (y * w + (w - 1 - x)) * c + ch;
                unsigned char temp = img->data[leftIdx];
                img->data[leftIdx] = img->data[rightIdx];
                img->data[rightIdx] = temp;
            }
        }
    }
}

void flipVertical(Image* img) {
    int w = img->width;
    int h = img->height;
    int c = img->channels;

    for (int y = 0; y < h / 2; ++y) {
        for (int x = 0; x < w; ++x) {
            for (int ch = 0; ch < c; ++ch) {
                int topIdx = (y * w + x) * c + ch;
                int bottomIdx = ((h - 1 - y) * w + x) * c + ch;
                unsigned char temp = img->data[topIdx];
                img->data[topIdx] = img->data[bottomIdx];
                img->data[bottomIdx] = temp;
            }
        }
    }
}

Image* rotateImage90(Image* src, int times) {
    times %= 4;
    if (times == 0) {
        // Return a copy if no rotation needed
        Image* copy = static_cast<Image*>(std::malloc(sizeof(Image)));
        *copy = *src;
        copy->data = static_cast<unsigned char*>(std::malloc(src->width * src->height * src->channels));
        std::memcpy(copy->data, src->data, src->width * src->height * src->channels);
        return copy;
    }

    Image* rotated = static_cast<Image*>(std::malloc(sizeof(Image)));
    rotated->channels = src->channels;
    rotated->max_val = src->max_val;

    int w = src->width;
    int h = src->height;
    unsigned char* s = src->data;

    if (times % 2 == 1) {
        rotated->width = h;
        rotated->height = w;
    } else {
        rotated->width = w;
        rotated->height = h;
    }

    rotated->data = static_cast<unsigned char*>(std::malloc(rotated->width * rotated->height * rotated->channels));

    int c = rotated->channels;

    if (times == 1) {
        // 90 degrees clockwise
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                for (int ch = 0; ch < c; ++ch) {
                    rotated->data[(x * rotated->width + (rotated->width - 1 - y)) * c + ch] = s[(y * w + x) * c + ch];
                }
            }
        }
    } else {
        // For other cases, rotate iteratively times times by 90 degrees
        Image* temp = nullptr;
        Image* current = rotated;

        // Create initial image copy for iterative rotations
        *rotated = *src;
        rotated->data = static_cast<unsigned char*>(std::malloc(w * h * c));
        std::memcpy(rotated->data, s, w * h * c);

        for (int t = 0; t < times; ++t) {
            temp = static_cast<Image*>(std::malloc(sizeof(Image)));
            temp->channels = current->channels;
            temp->max_val = current->max_val;
            temp->width = current->height;
            temp->height = current->width;
            temp->data = static_cast<unsigned char*>(std::malloc(temp->width * temp->height * temp->channels));

            for (int y = 0; y < temp->height; ++y) {
                for (int x = 0; x < temp->width; ++x) {
                    for (int ch = 0; ch < c; ++ch) {
                        int fromX = temp->width - 1 - x;
                        int fromY = y;
                        temp->data[(y * temp->width + x) * c + ch] = current->data[(fromX * current->width + fromY) * c + ch];
                    }
                }
            }
            std::free(current->data);
            if (t > 0) std::free(current);
            current = temp;
        }
        rotated = current;
    }
    return rotated;
}

void adjustIntensity(Image* img, int value) {
    int total = img->width * img->height * img->channels;
    for (int i = 0; i < total; ++i) {
        int newVal = img->data[i] + value;
        if (newVal > img->max_val) newVal = img->max_val;
        else if (newVal < 0) newVal = 0;
        img->data[i] = static_cast<unsigned char>(newVal);
    }
}

void releaseImage(Image* img) {
    if (img) {
        std::free(img->data);
        std::free(img);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::printf("Usage: %s input.ppm output.ppm operation [value]\n", argv[0]);
        std::puts("Operations:");
        std::puts("  negative");
        std::puts("  mirror_h");
        std::puts("  mirror_v");
        std::puts("  rotate <times_of_90_deg>");
        std::puts("  intensity <value>");
        return 1;
    }

    Image* img = loadImage(argv[1]);
    if (!img) {
        std::fprintf(stderr, "Error reading input image\n");
        return 1;
    }

    const char* op = argv[3];

    if (std::strcmp(op, "negative") == 0) {
        invertColors(img);
    } else if (std::strcmp(op, "mirror_h") == 0) {
        flipHorizontal(img);
    } else if (std::strcmp(op, "mirror_v") == 0) {
        flipVertical(img);
    } else if (std::strcmp(op, "rotate") == 0) {
        if (argc < 5) {
            std::fprintf(stderr, "Rotation requires an integer parameter\n");
            releaseImage(img);
            return 1;
        }
        int times = std::atoi(argv[4]);
        Image* rotated = rotateImage90(img, times);
        releaseImage(img);
        img = rotated;
    } else if (std::strcmp(op, "intensity") == 0) {
        if (argc < 5) {
            std::fprintf(stderr, "Intensity adjustment requires an integer parameter\n");
            releaseImage(img);
            return 1;
        }
        int val = std::atoi(argv[4]);
        adjustIntensity(img, val);
    } else {
        std::fprintf(stderr, "Unknown operation: %s\n", op);
        releaseImage(img);
        return 1;
    }

    saveImage(argv[2], img);
    releaseImage(img);

    return 0;
}
#include "Image.h"
#include <iostream>
#include <cstdio>
#include <cstring>

bool Image::load(const std::string& filename) {
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        perror("Unable to open file");
        return false;
    }

    char format[3];
    fscanf(file, "%2s", format);
    if (strcmp(format, "P6") != 0 && strcmp(format, "P5") != 0) {
        std::cerr << "Unsupported format (use P6/P5)\n";
        fclose(file);
        return false;
    }

    fscanf(file, "%d %d %d%*c", &width, &height, &max_val);
    channels = (format[1] == '6') ? 3 : 1;
    data.resize(width * height * channels);

    fread(data.data(), 1, data.size(), file);
    fclose(file);
    return true;
}

bool Image::save(const std::string& filename) const {
    FILE* file = fopen(filename.c_str(), "wb");
    if (!file) {
        perror("Unable to open file for writing");
        return false;
    }

    if (channels == 3)
        fprintf(file, "P6\n%d %d\n%d\n", width, height, max_val);
    else
        fprintf(file, "P5\n%d %d\n%d\n", width, height, max_val);

    fwrite(data.data(), 1, data.size(), file);
    fclose(file);
    return true;
}
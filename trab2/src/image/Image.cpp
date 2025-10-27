#include "../includes/image/Image.hpp"
#include <iostream>
#include <cstdio>
#include <cstring>

bool Image::load(const std::string& filename) {
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        perror("Unable to open file");
        return false;
    }

    char format[3] = {0};
    if (fscanf(file, "%2s", format) != 1) {
        std::cerr << "Failed to read image format\n";
        fclose(file);
        return false;
    }
    if (strcmp(format, "P6") != 0) {
        std::cerr << "Unsupported format (only P6 PPM supported)\n";
        fclose(file);
        return false;
    }

    if (fscanf(file, "%d %d %d%*c", &width, &height, &max_val) != 3) {
        std::cerr << "Failed to read image dimensions or max_val\n";
        fclose(file);
        return false;
    }
    channels = 3;
    data.resize(width * height * channels);

    size_t read_bytes = fread(data.data(), 1, data.size(), file);
    if (read_bytes != data.size()) {
        std::cerr << "Failed to read image data\n";
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

bool Image::save(const std::string& filename) const {
    FILE* file = fopen(filename.c_str(), "wb");
    if (!file) {
        perror("Unable to open file for writing");
        return false;
    }

    fprintf(file, "P6\n%d %d\n%d\n", width, height, max_val);
    size_t written_bytes = fwrite(data.data(), 1, data.size(), file);
    if (written_bytes != data.size()) {
        std::cerr << "Failed to write image data\n";
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}
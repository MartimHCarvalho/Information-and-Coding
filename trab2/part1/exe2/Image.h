#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <vector>

class Image {
public:
    int width;
    int height;
    int max_val;
    int channels;
    std::vector<unsigned char> data;

    // Constructors and destructors
    Image() = default;
    ~Image() = default;

    bool load(const std::string& filename);
    bool save(const std::string& filename) const;

    size_t size() const { return width * height * channels; }
};

#endif
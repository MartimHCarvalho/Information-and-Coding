#include "safetensors_parser.hpp"
#include <fstream>
#include <iostream>

SafetensorsParser::SafetensorsParser() : file_size_(0), header_size_(0) {}

SafetensorsParser::~SafetensorsParser() {}

bool SafetensorsParser::parse(const std::string& filepath) {
    filepath_ = filepath;

    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filepath << std::endl;
        return false;
    }

    file_size_ = file.tellg();
    file.seekg(0, std::ios::beg);

    std::cout << "Parsing: " << filepath << " (" << (file_size_ / 1024.0 / 1024.0) << " MB)" << std::endl;

    uint64_t header_size_le;
    file.read(reinterpret_cast<char*>(&header_size_le), 8);
    header_size_ = header_size_le;

    std::vector<uint8_t> header_bytes(header_size_);
    file.read(reinterpret_cast<char*>(header_bytes.data()), header_size_);
    header_ = std::string(header_bytes.begin(), header_bytes.end());

    size_t tensor_data_size = file_size_ - 8 - header_size_;
    tensor_data_.resize(tensor_data_size);
    file.read(reinterpret_cast<char*>(tensor_data_.data()), tensor_data_size);

    std::cout << "Header: " << header_size_ << " bytes, Tensors: "
              << (tensor_data_size / 1024.0 / 1024.0) << " MB" << std::endl;

    file.close();
    return true;
}

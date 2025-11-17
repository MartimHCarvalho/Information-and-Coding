#ifndef SAFETENSORS_PARSER_HPP
#define SAFETENSORS_PARSER_HPP

#include <string>
#include <vector>
#include <cstdint>

class SafetensorsParser {
public:
    SafetensorsParser();
    ~SafetensorsParser();

    bool parse(const std::string& filepath);

    const std::string& getHeader() const { return header_; }
    const std::vector<uint8_t>& getTensorData() const { return tensor_data_; }
    size_t getFileSize() const { return file_size_; }
    size_t getHeaderSize() const { return header_size_; }
    size_t getTensorDataSize() const { return tensor_data_.size(); }

private:
    std::string filepath_;
    std::string header_;
    std::vector<uint8_t> tensor_data_;
    size_t file_size_;
    size_t header_size_;
};

#endif

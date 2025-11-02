#include "../includes/bitstream.hpp"
#include <stdexcept>
#include <bitset>

BitStream::BitStream(const std::string& fileName, std::ios::openmode mode) : buffer(0), bitPosition(0) {
    file.open(fileName, mode | std::ios::binary);
    if (!file.is_open())
        throw std::ios_base::failure("Failed to open file.");
}

BitStream::~BitStream() {
    if (file.is_open()) {
        flushBuffer();
        file.close();
    }
}

void BitStream::writeBit(bool bit) {
    buffer |= (bit << (7 - bitPosition));
    bitPosition++;

    if (bitPosition == 8)
        flushBuffer();
}

bool BitStream::readBit() {
    if (bitPosition == 8 || bitPosition == 0)
        fillBuffer();

    bool bit = (buffer >> (7 - bitPosition)) & 1;
    bitPosition++;
    return bit;
}

void BitStream::writeBits(uint64_t value, int n) {
    if (n <= 0 || n > 64)
        throw std::invalid_argument("Number of bits must be between 1 and 64.");

    for (int i = n - 1; i >= 0; --i)
        writeBit((value >> i) & 1);
}

uint64_t BitStream::readBits(int n) {
    if (n <= 0 || n > 64)
        throw std::invalid_argument("Number of bits must be between 1 and 64.");

    uint64_t value = 0;
    for (int i = 0; i < n; ++i)
        value = (value << 1) | readBit();

    return value;
}

void BitStream::writeString(const std::string& str) {
    for (char c : str)
        writeBits(static_cast<uint8_t>(c), 8);
    writeBits(0, 8);
}

std::string BitStream::readString() {
    std::string result;
    uint64_t value = readBits(8);
    while (file.peek() != EOF && value != 0) {
        char c = static_cast<char>(value);
        result += c;
        value = readBits(8);
    }
    return result;
}

void BitStream::flushBuffer() {
    if (bitPosition > 0) {
        file.put(buffer);
        buffer = 0;
        bitPosition = 0;
    }
}

void BitStream::fillBuffer() {
    bitPosition = 0;

    if (file.peek() == EOF) {
        buffer = 0;
        return;
    }

    buffer = file.get();
}

void BitStream::close() {
    if (file.is_open()) {
        flushBuffer();
        file.close();
    }
}

bool BitStream::eof() const {
    return file.eof();
}
#ifndef BITSTREAM_HPP
#define BITSTREAM_HPP

#include <fstream>
#include <string>
#include <vector>
#include <cstdint>

class BitStream {
    private:
        std::fstream file;
        char buffer;
        int bitPosition; // Tracks the bit position within the current byte

        void flushBuffer();  // Writes remaining bits in the buffer to the file
        void fillBuffer();   // Fills the buffer from the file when reading bits

    public:
        BitStream(const std::string& fileName, std::ios::openmode mode);
        ~BitStream();

        void writeBit(bool bit);
        bool readBit();

        void writeBits(uint64_t value, int n);
        uint64_t readBits(int n);

        void writeString(const std::string& str);
        std::string readString();

        bool eof() const; // Check if the end of the file has been reached

        void close(); // Close the file safely
};

#endif // BITSTREAM_HPP

#ifndef BITSTREAM_HPP
#define BITSTREAM_HPP

#include <fstream>
#include <string>
#include <vector>
#include <cstdint>

class BitStream
{
private:
    std::fstream file;
    uint8_t writeBitBuffer; // Current byte being built for writing
    int writeBitPos;        // Position in writeBitBuffer (0-7, 0=empty)

    uint8_t readBitBuffer; // Current byte being read
    int readBitPos;        // Position in readBitBuffer (0-7, 0=need new byte)
    bool isEOF;

    void flushWriteBuffer();
    void fillReadBuffer();

public:
    BitStream(const std::string &fileName, std::ios::openmode mode);
    ~BitStream();

    void writeBit(bool bit);
    bool readBit();

    void writeBits(uint64_t value, int n);
    uint64_t readBits(int n);

    void writeString(const std::string &str);
    std::string readString();

    bool eof() const;
    void close();

    void flush();
    
    // Align bitstream to next byte boundary
    void alignToByte();
};

#endif // BITSTREAM_HPP
// ==================== bitstream.hpp ====================
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

// ==================== bitstream.cpp ====================
#include "../includes/bitstream.hpp"
#include <stdexcept>

BitStream::BitStream(const std::string &fileName, std::ios::openmode mode)
    : writeBitBuffer(0), writeBitPos(0), readBitBuffer(0), readBitPos(0), isEOF(false)
{
    file.open(fileName, mode | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Cannot open file: " + fileName);
    }
}

BitStream::~BitStream()
{
    close();
}

void BitStream::flushWriteBuffer()
{
    if (writeBitPos > 0)
    {
        file.put(writeBitBuffer);
        writeBitBuffer = 0;
        writeBitPos = 0;
    }
}

void BitStream::fillReadBuffer()
{
    if (file.eof())
    {
        isEOF = true;
        readBitBuffer = 0;
        readBitPos = 0;
        return;
    }

    int byte = file.get();
    if (byte == EOF)
    {
        isEOF = true;
        readBitBuffer = 0;
        readBitPos = 0;
    }
    else
    {
        readBitBuffer = static_cast<uint8_t>(byte);
        readBitPos = 0;
    }
}

void BitStream::writeBit(bool bit)
{
    if (bit)
    {
        writeBitBuffer |= (1 << (7 - writeBitPos));
    }
    writeBitPos++;

    if (writeBitPos == 8)
    {
        flushWriteBuffer();
    }
}

bool BitStream::readBit()
{
    if (readBitPos == 0)
    {
        fillReadBuffer();
    }

    if (isEOF)
    {
        return false;
    }

    bool bit = (readBitBuffer & (1 << (7 - readBitPos))) != 0;
    readBitPos++;

    if (readBitPos == 8)
    {
        readBitPos = 0;
    }

    return bit;
}

void BitStream::writeBits(uint64_t value, int n)
{
    if (n < 1 || n > 64)
        throw std::runtime_error("Invalid number of bits to write");

    // Write from MSB to LSB
    for (int i = n - 1; i >= 0; i--)
    {
        writeBit((value >> i) & 1ULL);
    }
}

uint64_t BitStream::readBits(int n)
{
    if (n < 1 || n > 64)
        throw std::runtime_error("Invalid number of bits to read");

    uint64_t value = 0;
    for (int i = 0; i < n; i++)
    {
        value = (value << 1) | (readBit() ? 1ULL : 0ULL);
    }
    return value;
}

void BitStream::writeString(const std::string &str)
{
    writeBits(str.length(), 32);
    for (char c : str)
    {
        writeBits(static_cast<uint8_t>(c), 8);
    }
}

std::string BitStream::readString()
{
    uint32_t length = readBits(32);
    std::string str;
    str.reserve(length);

    for (uint32_t i = 0; i < length; i++)
    {
        str += static_cast<char>(readBits(8));
    }

    return str;
}

bool BitStream::eof() const
{
    return isEOF;
}

void BitStream::close()
{
    flush();
    if (file.is_open())
    {
        file.close();
    }
}

void BitStream::flush()
{
    // Only flush if we're in write mode
    if (writeBitPos > 0)
    {
        // Pad remaining bits with zeros
        while (writeBitPos > 0)
        {
            writeBit(false);
        }
    }
    file.flush();
}

void BitStream::alignToByte()
{
    // For writing: flush any partial byte (padding with zeros)
    if (writeBitPos > 0)
    {
        // Pad remaining bits with zeros and flush
        while (writeBitPos > 0)
        {
            writeBit(false);
        }
    }
    
    // For reading: skip to next byte boundary
    if (readBitPos > 0)
    {
        // Discard remaining bits in current byte
        // Next readBit() call will load a fresh byte
        readBitPos = 0;
    }
}
#include "golomb.hpp"
#include <cmath>
#include <iostream>

Golomb::Golomb(int m, HandleSignApproach approach)
    : m(m), approach(approach)
{
    updateParameters();
}

Golomb::~Golomb() {}

void Golomb::updateParameters()
{
    if (m <= 0)
        m = 1;

    // Calculate b = ceil(log2(m))
    b = 0;
    int temp = m - 1;
    while (temp > 0)
    {
        b++;
        temp >>= 1;
    }

    // Calculate k = 2^b - m
    k = (1 << b) - m;
}

int Golomb::encode(int value, BitStream &bs)
{
    // Handle sign FIRST (critical for decode order)
    int absValue;
    if (approach == HandleSignApproach::SIGN_AND_MAGNITUDE)
    {
        if (value < 0)
        {
            bs.writeBit(1);
            absValue = -value;
        }
        else
        {
            bs.writeBit(0);
            absValue = value;
        }
    }
    else
    { // ODD_EVEN_MAPPING
        if (value >= 0)
        {
            absValue = 2 * value;
        }
        else
        {
            absValue = -2 * value - 1;
        }
    }

    // Golomb encoding: q = absValue / m, r = absValue % m
    int q = absValue / m;
    int r = absValue % m;

    // Write unary code for quotient (q zeros followed by a 1)
    for (int i = 0; i < q; i++)
    {
        bs.writeBit(0);
    }
    bs.writeBit(1);

    // Write truncated binary code for remainder
    if (r < k)
    {
        bs.writeBits(r, b - 1);
    }
    else
    {
        bs.writeBits(r + k, b);
    }

    return q + 1 + (r < k ? b - 1 : b);
}

int Golomb::decode(BitStream &bs)
{
    bool isNegative = false;
    if (approach == HandleSignApproach::SIGN_AND_MAGNITUDE)
    {
        if (bs.eof())
            return 0;
        isNegative = bs.readBit();
    }

    // Unary decode: count zeros until we hit a 1
    int q = 0;
    while (!bs.readBit() && !bs.eof())
    {
        q++;
        if (q > 100000) // Safety check
        {
            std::cerr << "Error: Quotient too large" << std::endl;
            return 0;
        }
    }

    // Read remainder - CRITICAL FIX: correct truncated binary decoding
    int r = 0;

    if (k == 0)
    {
        // Power of 2: just read b bits
        r = bs.readBits(b);
    }
    else
    {
        // Truncated binary: read b-1 bits first
        r = bs.readBits(b - 1);

        // If r >= k, we need to read one more bit and adjust
        if (r >= k)
        {
            int extraBit = bs.readBit() ? 1 : 0;
            r = ((r << 1) | extraBit) - k;
        }
    }

    int absValue = q * m + r;

    if (approach == HandleSignApproach::SIGN_AND_MAGNITUDE)
    {
        return isNegative ? -absValue : absValue;
    }
    else
    {
        // ODD_EVEN_MAPPING: branchless demapping
        if (absValue & 1)
            return -((absValue + 1) >> 1);
        else
            return absValue >> 1;
    }
}

// OPTIMIZATION 5: Specialized decoder for power-of-2 m values
int Golomb::decodePow2(BitStream &bs)
{
    // When m is a power of 2 (k == 0), decoding is much simpler

    bool isNegative = false;
    if (approach == HandleSignApproach::SIGN_AND_MAGNITUDE)
    {
        if (bs.eof())
            return 0;
        isNegative = bs.readBit();
    }

    // Unary decode for quotient
    int q = 0;
    while (!bs.readBit())
    {
        q++;
        if (q > 10000)
            return 0;
    }

    // Direct read of remainder (no truncation)
    int r = bs.readBits(b);

    // Fast multiply using shift: q * m + r where m = 2^b
    int absValue = (q << b) | r;

    if (approach == HandleSignApproach::SIGN_AND_MAGNITUDE)
    {
        return isNegative ? -absValue : absValue;
    }
    else
    {
        // Branchless odd/even demapping
        return (absValue & 1) ? -((absValue + 1) >> 1) : (absValue >> 1);
    }
}

// OPTIMIZATION 6: Batch decode for multiple values
int Golomb::decodeBatch(BitStream &bs, int *output, int count)
{
    // Decode multiple values with improved cache locality
    bool isPow2 = (k == 0);

    for (int i = 0; i < count; i++)
    {
        if (bs.eof())
            return i;
        output[i] = isPow2 ? decodePow2(bs) : decode(bs);
    }
    return count;
}

void Golomb::setM(int newM)
{
    m = newM;
    updateParameters();
}

int Golomb::getM() const
{
    return m;
}
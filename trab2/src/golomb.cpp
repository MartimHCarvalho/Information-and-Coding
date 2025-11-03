#include "golomb.hpp"
#include "bitstream.hpp"
#include <cmath>
#include <stdexcept>

Golomb::Golomb(int m, HandleSignApproach approach)
    : m(m), b(0), k(0), approach(approach) {
    if (m <= 0) {
        throw std::invalid_argument("Parameter m must be greater than 0.");
    }
    updateParameters();
}

Golomb::~Golomb() {}

void Golomb::updateParameters() {
    if (m == 1) {
        b = 0;
        k = 0;
    } else {
        b = static_cast<int>(std::log2(m));
        k = (1 << (b + 1)) - m;
    }
}

int Golomb::encode(int value, BitStream& bs) {
    int bits = 0;
    int mappedValue = value;

    // Handle sign based on selected approach
    if (approach == HandleSignApproach::SIGN_AND_MAGNITUDE) {
        if (value < 0) {
            bs.writeBit(1);
            mappedValue = -value;
            bits++;
        } else {
            bs.writeBit(0);
            bits++;
        }
    } else if (approach == HandleSignApproach::ODD_EVEN_MAPPING) {
        if (value > 0) {
            mappedValue = 2 * value - 1;
        } else {
            mappedValue = static_cast<int>(-2LL * value);
        }
    }

    // Calculate quotient and remainder
    int q = mappedValue / m;
    int r = mappedValue % m;

    // Write unary quotient
    for (int i = 0; i < q; ++i) {
        bs.writeBit(1);
        bits++;
    }
    bs.writeBit(0);
    bits++;

    // Write truncated binary remainder
    if (m == 1) {
        // No remainder to encode
    } else if (r < k) {
        // Use b bits
        for (int i = b - 1; i >= 0; --i) {
            bs.writeBit((r >> i) & 1);
            bits++;
        }
    } else {
        // Use b+1 bits
        int encodedR = r + k;
        for (int i = b; i >= 0; --i) {
            bs.writeBit((encodedR >> i) & 1);
            bits++;
        }
    }

    return bits;
}

int Golomb::decode(BitStream& bs) {
    int signBit = 0;
    int value = 0;

    // Read sign bit if using SIGN_AND_MAGNITUDE approach
    if (approach == HandleSignApproach::SIGN_AND_MAGNITUDE) {
        signBit = bs.readBit();
    }

    // Read unary quotient
    int q = 0;
    while (!bs.eof() && bs.readBit()) {
        q++;
    }

    // Read truncated binary remainder
    int r = 0;
    if (m == 1) {
        r = 0;
    } else {
        // Read b bits
        for (int i = 0; i < b; ++i) {
            if (bs.eof()) break;
            r = (r << 1) | (bs.readBit() ? 1 : 0);
        }

        // Check if we need to read an extra bit
        if (r >= k && !bs.eof()) {
            r = (r << 1) | (bs.readBit() ? 1 : 0);
            r = r - k;
        }
    }

    // Reconstruct mapped value
    int mappedValue = q * m + r;

    // Convert back based on approach
    if (approach == HandleSignApproach::SIGN_AND_MAGNITUDE) {
        value = (signBit == 1) ? -mappedValue : mappedValue;
    } else if (approach == HandleSignApproach::ODD_EVEN_MAPPING) {
        if (mappedValue % 2 == 0) {
            value = -(mappedValue / 2);
        } else {
            value = (mappedValue + 1) / 2;
        }
    } else {
        throw std::runtime_error("Invalid HandleSignApproach");
    }

    return value;
}

void Golomb::setM(int newM) {
    if (newM <= 0) {
        throw std::invalid_argument("Parameter m must be greater than 0.");
    }
    this->m = newM;
    updateParameters();
}

int Golomb::getM() const {
    return m;
}

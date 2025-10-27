#include "../includes/Part_II/golomb.hpp"
#include "../includes/Part_II/bitstream.hpp"
#include <stdexcept>
#include <cmath>

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
        // Special case: m=1 means pure unary coding, no remainder needed
        b = 0;
        k = 0;
    } else {
        // Calculate b = floor(log2(m))
        b = static_cast<int>(std::log2(m));
        // Calculate k = 2^(b+1) - m
        k = (1 << (b + 1)) - m;
    }
}

int Golomb::encode(int value, BitStream& bs) {
    int bits = 0;
    int mappedValue = value;
    
    // Handle negative numbers based on the chosen approach
    if (approach == HandleSignApproach::SIGN_AND_MAGNITUDE) {
        // Approach 1: Sign bit + magnitude
        if (value < 0) {
            bs.writeBit(1);  // Sign bit = 1 for negative
            mappedValue = -value;
            bits++;
        } else {
            bs.writeBit(0);  // Sign bit = 0 for positive/zero
            bits++;
        }
    } else if (approach == HandleSignApproach::ODD_EVEN_MAPPING) {
        // Approach 2: Interleaving mapping
        // Positive values (x >= 1): x -> 2x - 1 (odd numbers: 1, 3, 5, 7, ...)
        // Zero and negative values: 0 -> 0, -1 -> 2, -2 -> 4, -3 -> 6, ...
        // Formula: x > 0 -> 2x - 1; x <= 0 -> -2x
        if (value > 0) {
            mappedValue = 2 * value - 1;
        } else {
            // Safe conversion to avoid overflow with INT_MIN
            mappedValue = static_cast<int>(-2LL * value);
        }
    }
    
    // Calculate quotient and remainder
    int q = mappedValue / m;
    int r = mappedValue % m;
    
    // Encode quotient in unary: q ones followed by a zero
    for (int i = 0; i < q; ++i) {
        bs.writeBit(1);
        bits++;
    }
    bs.writeBit(0);  // Terminating zero
    bits++;
    
    // Encode remainder using truncated binary encoding
    if (m == 1) {
        // No remainder to encode when m=1 (pure unary)
    } else if (r < k) {
        // Use b bits for remainder
        for (int i = b - 1; i >= 0; --i) {
            bs.writeBit((r >> i) & 1);
            bits++;
        }
    } else {
        // Use b+1 bits, encoding (r + k)
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
    
    // Read sign bit if using sign-and-magnitude approach
    if (approach == HandleSignApproach::SIGN_AND_MAGNITUDE) {
        signBit = bs.readBit();
    }
    
    // Decode quotient: count consecutive ones until we hit a zero
    int q = 0;
    while (!bs.eof() && bs.readBit()) {
        q++;
    }
    
    // Decode remainder using truncated binary encoding
    int r = 0;
    
    if (m == 1) {
        // No remainder when m=1 (pure unary)
        r = 0;
    } else {
        // Read first b bits
        for (int i = 0; i < b; ++i) {
            if (bs.eof()) break;
            r = (r << 1) | (bs.readBit() ? 1 : 0);
        }
        
        // Check if we need to read one more bit
        if (r >= k && !bs.eof()) {
            // Read one more bit and adjust
            r = (r << 1) | (bs.readBit() ? 1 : 0);
            r = r - k;
        }
    }
    
    // Reconstruct the mapped value
    int mappedValue = q * m + r;
    
    // Convert back to original value based on approach
    int value;
    if (approach == HandleSignApproach::SIGN_AND_MAGNITUDE) {
        // Apply sign
        value = (signBit == 1) ? -mappedValue : mappedValue;
    } else if (approach == HandleSignApproach::ODD_EVEN_MAPPING) {
        // Reverse the interleaving mapping
        // Even numbers (0, 2, 4, 6, ...) -> 0, -1, -2, -3, ...
        // Odd numbers (1, 3, 5, 7, ...) -> 1, 2, 3, 4, ...
        if (mappedValue % 2 == 0) {
            // Even: mapped to zero or negative
            value = -(mappedValue / 2);
        } else {
            // Odd: mapped to positive
            value = (mappedValue + 1) / 2;
        }
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

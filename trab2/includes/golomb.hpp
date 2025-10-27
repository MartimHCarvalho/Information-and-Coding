#ifndef GOLOMB_HPP
#define GOLOMB_HPP

#include <string>
#include "bitstream.hpp"

class Golomb {
private:
    int m;          // Parameter m for Golomb encoding
    int b;          // Number of bits for truncated binary (calculated from m)
    int k;          // Cutoff value for truncated binary (calculated from m)

public:
    enum class HandleSignApproach {
        SIGN_AND_MAGNITUDE,      // Approach 1: sign bit + magnitude
        ODD_EVEN_MAPPING         // Approach 2: positive/negative interleaving
    };

    // Constructor
    Golomb(int m, HandleSignApproach approach);
    
    // Destructor
    ~Golomb();

    // Encode an integer value and write to bitstream
    int encode(int value, BitStream& bs);

    // Decode an integer value from bitstream
    int decode(BitStream& bs);

    // Change the m parameter
    void setM(int m);

    // Get current m parameter
    int getM() const;

private:
    HandleSignApproach approach;
    
    // Helper function to update b and k when m changes
    void updateParameters();
};

#endif

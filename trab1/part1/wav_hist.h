#ifndef WAVHIST_H
#define WAVHIST_H

#include <iostream>
#include <vector>
#include <map>
#include <sndfile.hh>

class WAVHist {
  private:
    // Original histogram storage for individual channels (L, R, etc.)
    std::vector<std::map<short, size_t>> counts;
    
    // Additional histogram for MID channel: (L + R) / 2
    // This represents the mono version of stereo audio
    std::map<short, size_t> midCounts;
    
    // Additional histogram for SIDE channel: (L - R) / 2  
    // This represents the difference between channels, used in stereo coding
    std::map<short, size_t> sideCounts;
    
    // Store number of channels to determine if we have stereo (2 channels)
    size_t numChannels;


    
    // Store 2^k values per bin
    unsigned binShift_;
    inline short binKey(short s) const {
        if (binShift_ == 0) return s;
        const unsigned mask = (~((1u << binShift_) - 1u)) & 0xFFFFu; // 16-bit
        unsigned u = static_cast<unsigned>(static_cast<uint16_t>(s));
        return static_cast<short>(u & mask);
    }


  public:
    WAVHist(const SndfileHandle& sfh, unsigned binShift = 0) 
        : numChannels(sfh.channels()), binShift_(binShift > 15 ? 15 : binShift) {
        // Resize the counts vector to accommodate all original channels
        counts.resize(sfh.channels());
        // midCounts and sideCounts are initialized empty and will only be used for stereo
    }

    void update(const short* data, size_t nSamples) {
        const size_t ch = numChannels;
        const size_t nFrames = (ch ? nSamples / ch : 0);

        for (size_t f = 0; f < nFrames; ++f) {
            const size_t base = f * ch;

            // Per-channel histograms
            for (size_t c = 0; c < ch; ++c) {
                short v = data[base+c];
                counts[c][binKey(v)]++;   
            }

            // MID / SIDE only for stereo
            if (ch == 2) {
                int left  = static_cast<int>(data[base + 0]);
                int right = static_cast<int>(data[base + 1]);

                // Integer math to avoid overflow
                const int mid_i  = (left + right) / 2;
                const int side_i = (left - right) / 2;

                short mid  = static_cast<short>(mid_i);
                short side = static_cast<short>(side_i);

                midCounts[binKey(mid)]++; 
                sideCounts[binKey(side)]++;
            }
        }
    }

    void update(const std::vector<short>& samples) {
        update(samples.data(), samples.size());
    }

    void dump(const size_t channel) const {
        if(channel < numChannels) {
            // Dump histogram for original channels (0 = left, 1 = right, etc.)
            for(auto [value, counter] : counts[channel])
                std::cout << value << '\t' << counter << '\n';
        } 
        else if(channel == numChannels && numChannels == 2) {
            // Dump MID channel histogram (channel index = 2 for stereo)
            // MID represents the average: (L + R) / 2
            for(auto [value, counter] : midCounts)
                std::cout << value << '\t' << counter << '\n';
        } 
        else if(channel == numChannels + 1 && numChannels == 2) {
            // Dump SIDE channel histogram (channel index = 3 for stereo)
            // SIDE represents the difference: (L - R) / 2
            for(auto [value, counter] : sideCounts)
                std::cout << value << '\t' << counter << '\n';
        } 
        else {
            // Invalid channel requested
            std::cerr << "Error: invalid channel requested\n";
        }
    }

    // Helper function to get the number of original channels
    size_t getNumChannels() const {
        return numChannels;
    }

    // Helper function to check if MID/SIDE channels are available
    bool hasMidSide() const {
        return numChannels == 2;
    }
};

#endif
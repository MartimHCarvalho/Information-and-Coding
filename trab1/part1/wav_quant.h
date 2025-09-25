#ifndef WAVQUANT_H
#define WAVQUANT_H

#include <iostream>
#include <vector>
#include <array>
#include <sndfile.hh>

class WAVQuant
{
private:
    std::vector<short> quant_samples;

public:
    WAVQuant()
    {
        quant_samples.resize(0);
    }

    void quant(const std::vector<short> &samples, size_t num_bits_to_cut)
    {
        for (auto sample : samples)
        {
            sample = sample >> num_bits_to_cut;    // Shift right to cut bits
            short tmp = sample << num_bits_to_cut; // Shift left back to original position with cut bits zeroed
            quant_samples.push_back(tmp);
        }
    }

    void to_Wav(SndfileHandle &sfhOut)
    {
        sfhOut.write(quant_samples.data(), quant_samples.size());
    }

    std::array<size_t, 256> computeHistogram() const
    {
        std::array<size_t, 256> histogram = {};
        for (auto sample : quant_samples)
        {
            size_t bin = static_cast<size_t>((static_cast<int>(sample) + 32768) / 256);
            if (bin >= histogram.size())
                bin = histogram.size() - 1;
            histogram[bin]++;
        }
        return histogram;
    }

    void printHistogram() const
    {
        auto histogram = computeHistogram();
        for (size_t i = 0; i < histogram.size(); i++)
        {
            int bin_center = static_cast<int>(i * 256 - 32768 + 128); 
            std::cout << bin_center << " " << histogram[i] << std::endl;
        }
    }
};

#endif

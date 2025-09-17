#ifndef WAVQUANT_H
#define WAVQUANT_H

#include <iostream>
#include <vector>
#include <map>
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
            sample = sample >> num_bits_to_cut; // take each short sample and turn into 0 the num_bits_to_cut least significant bits
            short tmp = sample << num_bits_to_cut; // shift the sample back to its original position
            quant_samples.insert(quant_samples.end(), tmp); // add the sample to the vector
        }
    }

    void to_Wav(SndfileHandle &sfhOut)
    {
        sfhOut.write(quant_samples.data(), quant_samples.size());
    }
};

#endif
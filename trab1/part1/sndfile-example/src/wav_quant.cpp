#include <iostream>
#include <vector>
#include <sndfile.hh>
#include "wav_quant.h"

using namespace std;

constexpr size_t FRAMES_BUFFER_SIZE = 65536; // Buffer for reading frames

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        cerr << "Usage: " << argv[0] << " <input file> <target_bits> <output_file>\n";
        return 1;
    }

    SndfileHandle sndFile{argv[argc - 3]};
    if (sndFile.error())
    {
        cerr << "Error: invalid input file\n";
        return 1;
    }

    if ((sndFile.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV)
    {
        cerr << "Error: file is not in WAV format\n";
        return 1;
    }

    SndfileHandle sfhOut{argv[argc - 1], SFM_WRITE, sndFile.format(), sndFile.channels(), sndFile.samplerate()};
    if (sfhOut.error())
    {
        cerr << "Error: invalid output file\n";
        return 1;
    }

    if ((sndFile.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16)
    {
        cerr << "Error: file is not in PCM_16 format\n";
        return 1;
    }

    int goal_bits{stoi(argv[argc - 2])};

    if (goal_bits < 1 || goal_bits > 16)
    {
        cerr << "Error: invalid number of bits to represent the audio sample\n";
        return 1;
    }

    size_t num_bits_to_cut = 16 - goal_bits;
    vector<short> samples(FRAMES_BUFFER_SIZE * sndFile.channels());
    WAVQuant quant{};

    size_t nFrames;
    while ((nFrames = sndFile.readf(samples.data(), FRAMES_BUFFER_SIZE)))
    {
        samples.resize(nFrames * sndFile.channels());
        quant.quant(samples, num_bits_to_cut);
    }
    quant.to_Wav(sfhOut);
    quant.printHistogram();

    return 0;
}

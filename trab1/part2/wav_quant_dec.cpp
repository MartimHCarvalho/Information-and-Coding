#include <iostream>
#include <vector>
#include <fstream>
#include <sndfile.hh>
#include "../part2/bit_stream/src/bit_stream.h"

constexpr size_t FRAMES_BUFFER_SIZE = 65536;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " input.pack output.wav\n";
        return 1;
    }

    std::fstream ifs(argv[1], std::ios::in | std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "Error opening input file\n";
        return 1;
    }
    BitStream bstream(ifs, true);

    // read header info
    int goalBits = bstream.read_n_bits(5);
    int channels = bstream.read_n_bits(4);
    int sampleRate = bstream.read_n_bits(20);
    int shift = 16 - goalBits;

    SF_INFO sfinfo;
    sfinfo.channels = channels;
    sfinfo.samplerate = sampleRate;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SndfileHandle sfhOut(argv[2], SFM_WRITE, sfinfo.format, sfinfo.channels, sfinfo.samplerate);
    if (sfhOut.error()) {
        std::cerr << "Error opening output .wav file\n";
        return 1;
    }

    std::vector<short> samples;
    while (true) {
        if (ifs.eof()) break;

        int packed;
        try {
            packed = bstream.read_n_bits(goalBits);
        } catch (...) {
            break; 
        }

        short restored = packed << shift;
        samples.push_back(restored);

        if (samples.size() >= FRAMES_BUFFER_SIZE) {
            sfhOut.write(samples.data(), samples.size());
            samples.clear();
        }
    }

    if (!samples.empty()) {
        sfhOut.write(samples.data(), samples.size());
    }

    bstream.close();
    ifs.close();
    return 0;
}
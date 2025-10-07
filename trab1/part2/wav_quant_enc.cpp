#include <iostream>
#include <vector>
#include <fstream>
#include <sndfile.hh>
#include "../bit_stream/src/bit_stream.h"

constexpr size_t FRAMES_BUFFER_SIZE = 65536;

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " input.wav targetbits output.pack\n";
        return 1;
    }

    SndfileHandle sndFile(argv[1]);
    if (sndFile.error()) {
        std::cerr << "Error: invalid input file\n";
        return 1;
    }
    if ((sndFile.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV ||
        (sndFile.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
        std::cerr << "Error: file is not WAV PCM16\n";
        return 1;
    }

    int goalBits = std::stoi(argv[2]);
    if (goalBits < 1 || goalBits > 16) {
        std::cerr << "Error: invalid number of bits\n";
        return 1;
    }

    std::fstream ofs(argv[3], std::ios::out | std::ios::binary);
    if (!ofs.is_open()) {
        std::cerr << "Error opening output file\n";
        return 1;
    }

    BitStream bstream(ofs, false);

    bstream.write_n_bits(goalBits, 5);
    bstream.write_n_bits(sndFile.channels(), 4);
    bstream.write_n_bits(sndFile.samplerate(), 20);
    bstream.write_n_bits(static_cast<uint32_t>(sndFile.frames()), 32);

    std::vector<short> samples(FRAMES_BUFFER_SIZE * sndFile.channels());
    sf_count_t framesRead;
    while ((framesRead = sndFile.readf(samples.data(), FRAMES_BUFFER_SIZE)) > 0) {
        size_t samplesCount = static_cast<size_t>(framesRead) * sndFile.channels();
        for (size_t i = 0; i < samplesCount; ++i) {
            int packed = samples[i] >> (16 - goalBits);
            bstream.write_n_bits(packed, goalBits);
        }
    }

    bstream.close();
    ofs.close();
    return 0;
}
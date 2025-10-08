#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <sndfile.hh>
#include "../bit_stream/src/bit_stream.h"

constexpr size_t BLOCK_SIZE = 1024;

// DCT-II
void dct(const std::vector<double>& input, std::vector<double>& output) {
    size_t N = input.size();
    for (size_t k = 0; k < N; k++) {
        double sum = 0.0;
        for (size_t n = 0; n < N; n++) {
            sum += input[n] * cos(M_PI * k * (2*n + 1) / (2 * N));
        }
        double alpha = (k == 0) ? sqrt(1.0 / N) : sqrt(2.0 / N);
        output[k] = alpha * sum;
    }
}

void quantize(const std::vector<double>& input, std::vector<int>& output, int qstep) {
    size_t N = input.size();
    for (size_t i = 0; i < N; i++) {
        output[i] = static_cast<int>(round(input[i] / qstep));
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " input.wav quantStep output.bin\n";
        return 1;
    }

    const char* inputFile = argv[1];
    int qstep = std::stoi(argv[2]);
    const char* outputFile = argv[3];

    SndfileHandle sndFile(inputFile);
    if (sndFile.error()) {
        std::cerr << "Error opening input file.\n";
        return 1;
    }
    if (sndFile.channels() != 1) {
        std::cerr << "Input file must be mono.\n";
        return 1;
    }

    std::fstream ofs(outputFile, std::ios::binary | std::ios::out);
    if (!ofs.is_open()) {
        std::cerr << "Error opening output file.\n";
        return 1;
    }

    BitStream bstream(ofs, false);

    // Header (block size, sample rate, channels=1, qstep, total samples)
    bstream.write_n_bits(BLOCK_SIZE, 16);
    bstream.write_n_bits(sndFile.samplerate(), 20);
    bstream.write_n_bits(1, 4);  // Fixed to mono
    bstream.write_n_bits(qstep, 8);
    bstream.write_n_bits((uint32_t)sndFile.frames(), 32); // total number of samples in 32 bits

    std::vector<short> samples(BLOCK_SIZE);
    std::vector<double> block(BLOCK_SIZE);
    std::vector<double> dctCoeffs(BLOCK_SIZE);
    std::vector<int> quantCoeffs(BLOCK_SIZE);

    size_t framesRead;
    while ((framesRead = sndFile.readf(samples.data(), BLOCK_SIZE)) > 0) {
        if (framesRead < BLOCK_SIZE) {
            std::fill(samples.begin() + framesRead, samples.end(), 0);
        }

        for (size_t i = 0; i < BLOCK_SIZE; i++) {
            block[i] = static_cast<double>(samples[i]);
        }

        dct(block, dctCoeffs);
        quantCoeffs.resize(BLOCK_SIZE);
        quantize(dctCoeffs, quantCoeffs, qstep);

        for (int coeff : quantCoeffs) {
            int valToWrite = coeff + 32768;
            bstream.write_n_bits(valToWrite, 16);
        }
    }

    bstream.close();
    ofs.close();
    return 0;
}

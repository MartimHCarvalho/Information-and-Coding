#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <sndfile.hh>
#include "../bit_stream/src/bit_stream.h"

constexpr size_t BLOCK_SIZE = 1024;

void idct(const std::vector<double>& input, std::vector<double>& output) {
    size_t N = input.size();
    for (size_t n = 0; n < N; n++) {
        double sum = 0.0;
        for (size_t k = 0; k < N; k++) {
            double alpha = (k == 0) ? sqrt(1.0 / N) : sqrt(2.0 / N);
            sum += alpha * input[k] * cos(M_PI * k * (2*n + 1) / (2 * N));
        }
        output[n] = sum;
    }
}

void dequantize(const std::vector<int>& input, std::vector<double>& output, int qstep) {
    size_t N = input.size();
    for (size_t i = 0; i < N; i++) {
        output[i] = static_cast<double>(input[i]) * qstep;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " input.bin output.wav\n";
        return 1;
    }

    const char* inputFile = argv[1];
    const char* outputFile = argv[2];

    std::fstream ifs(inputFile, std::ios::binary | std::ios::in);
    if (!ifs.is_open()) {
        std::cerr << "Error opening input file.\n";
        return 1;
    }

    BitStream bstream(ifs, true);

    size_t blockSize = bstream.read_n_bits(16);
    int sampleRate = bstream.read_n_bits(20);
    int channels = bstream.read_n_bits(4);
    int qstep = bstream.read_n_bits(8);
    uint32_t num_samples = bstream.read_n_bits(32); 

    if (channels != 1) {
        std::cerr << "Only mono supported.\n";
        return 1;
    }

    SF_INFO sfinfo;
    sfinfo.channels = channels;
    sfinfo.samplerate = sampleRate;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    sfinfo.frames = num_samples;

    SndfileHandle sfhOut(outputFile, SFM_WRITE, sfinfo.format, sfinfo.channels, sfinfo.samplerate);
    if (sfhOut.error()) {
        std::cerr << "Error opening output WAV.\n";
        return 1;
    }

    std::vector<int> quantCoeffs(blockSize);
    std::vector<double> dequantCoeffs(blockSize);
    std::vector<double> samples(blockSize);

    uint32_t totalWritten = 0;
    while (totalWritten < num_samples) {
        size_t curBlock = std::min<size_t>(blockSize, num_samples - totalWritten);
        try {
            for (size_t i = 0; i < blockSize; i++) {
                int val = bstream.read_n_bits(16);
                quantCoeffs[i] = val - 32768;
            }
        } catch (...) {
            break;
        }

        dequantize(quantCoeffs, dequantCoeffs, qstep);
        idct(dequantCoeffs, samples);

        std::vector<short> outputSamples(curBlock);
        for (size_t i = 0; i < curBlock; i++) {
            double val = samples[i];
            if (val > 32767.0) val = 32767.0;
            else if (val < -32768.0) val = -32768.0;
            outputSamples[i] = static_cast<short>(val);
        }
        sfhOut.writef(outputSamples.data(), curBlock);
        totalWritten += curBlock;
    }

    return 0;
}
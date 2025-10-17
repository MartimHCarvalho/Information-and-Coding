#ifndef WAVCMP_H
#define WAVCMP_H

#include <vector>
#include <sndfile.hh>

struct WavCmpStats {
    std::vector<double> mse;       // Mean Squared Error per channel
    std::vector<double> maxerr;    // Maximum absolute error per channel
    std::vector<double> signal;    // Sum of squares per channel
    std::vector<double> noise;     // Sum of squared error per channel
    size_t channels;

    WavCmpStats(size_t ch) : mse(ch, 0.0), maxerr(ch, 0.0), signal(ch, 0.0), noise(ch, 0.0), channels(ch) {}
};

bool wav_cmp(const char *ref_path, const char *test_path, WavCmpStats &stats, size_t &num_samples);

#endif // WAVCMP_H

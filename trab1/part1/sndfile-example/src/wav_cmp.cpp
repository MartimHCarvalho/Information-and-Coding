#include <iostream>
#include <cmath>
#include <vector>
#include <iomanip>  // For formatting output
#include "wav_cmp.h"
#include <sndfile.hh>

using namespace std;

constexpr size_t FRAMESBUFFERSIZE = 65536; // Buffered read

bool wav_cmp(const char *ref_path, const char *test_path, WavCmpStats &stats, size_t &num_samples) {
    SndfileHandle ref_sf(ref_path);
    SndfileHandle test_sf(test_path);

    if (ref_sf.channels() != test_sf.channels() ||
        ref_sf.samplerate() != test_sf.samplerate() ||
        ref_sf.frames() != test_sf.frames())
    {
        cerr << "Input files must have same format, channels, and length." << endl;
        return false;
    }

    size_t channels = ref_sf.channels();
    stats = WavCmpStats(channels);
    num_samples = ref_sf.frames();

    vector<short> ref_buf(FRAMESBUFFERSIZE * channels);
    vector<short> test_buf(FRAMESBUFFERSIZE * channels);

    size_t total_samples = 0;

    while (total_samples < num_samples) {
        sf_count_t frames_to_read = min(FRAMESBUFFERSIZE, num_samples - total_samples);
        sf_count_t ref_read = ref_sf.readf(ref_buf.data(), frames_to_read);
        sf_count_t test_read = test_sf.readf(test_buf.data(), frames_to_read);

        if (ref_read != test_read || ref_read == 0)
            break;

        for (sf_count_t f = 0; f < ref_read; ++f) {
            for (size_t c = 0; c < channels; ++c) {
                size_t idx = f * channels + c;
                double x = ref_buf[idx];
                double y = test_buf[idx];
                double err = x - y;

                // Mean squared error
                stats.mse[c] += err * err;
                // Max absolute error
                stats.maxerr[c] = max(stats.maxerr[c], abs(err));
                // Signal power
                stats.signal[c] += x * x;
                // Noise power
                stats.noise[c] += err * err;
            }
        }
        total_samples += ref_read;
    }

    if (total_samples > 0) {
        for (size_t c = 0; c < channels; ++c) {
            stats.mse[c] /= total_samples;
        }
        num_samples = total_samples;
    }
    return (total_samples == num_samples);
}

double compute_average(const vector<double> &v) {
    double sum = 0;
    for (double x : v) sum += x;
    return sum / v.size();
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " original.wav test.wav" << endl;
        return 1;
    }

    WavCmpStats stats(0);
    size_t n_samples = 0;

    if (!wav_cmp(argv[1], argv[2], stats, n_samples)) {
        cerr << "File comparison failed." << endl;
        return 2;
    }

    // Print table header
    cout << left
         << setw(10) << "Channel"
         << setw(22) << "L2 (MSE)"
         << setw(22) << "L∞ (Max Abs Error)"
         << setw(15) << "SNR (dB)" << endl;

    // Print each channel's stats
    cout << fixed << setprecision(6);
    for (size_t c = 0; c < stats.channels; ++c) {
        double snr = (stats.noise[c] == 0.0) ? INFINITY : 10.0 * log10(stats.signal[c] / stats.noise[c]);
        cout << left
             << setw(10) << c
             << setw(22) << stats.mse[c]
             << setw(22) << stats.maxerr[c]
             << setw(15) << snr
             << endl;
    }

    // Print average across channels
    double avg_mse = compute_average(stats.mse);
    double avg_maxerr = compute_average(stats.maxerr);
    double avg_signal = compute_average(stats.signal);
    double avg_noise = compute_average(stats.noise);
    double avg_snr = (avg_noise == 0.0) ? INFINITY : 10.0 * log10(avg_signal / avg_noise);

    cout << left
         << setw(10) << "Average"
         << setw(22) << avg_mse
         << setw(22) << avg_maxerr
         << setw(15) << avg_snr << endl;

    return 0;
}

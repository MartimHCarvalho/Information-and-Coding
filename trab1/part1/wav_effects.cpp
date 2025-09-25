#include <iostream>
#include <vector>
#include <cmath>
#include <sndfile.hh>
#include "wav_effects.h"


using namespace std;

constexpr size_t FRAMES_BUFFER_SIZE = 65536;

// Apply single echo
void singleEcho(vector<short>& samples, int sampleRate, int channels, float delaySec, float decay) {
    int delaySamples = static_cast<int>(delaySec * sampleRate);
    vector<short> out(samples.size(), 0);

    for (size_t i = 0; i < samples.size(); ++i) {
        out[i] = samples[i];
        if (i >= static_cast<size_t>(delaySamples) * static_cast<size_t>(channels)) {
            int delayed = static_cast<int>(samples[i - delaySamples * channels] * decay);
            int mixed = static_cast<int>(out[i]) + delayed;
            out[i] = static_cast<short>(max(min(mixed, 32767), -32768));
        }
    }
    samples = out;
}

// Apply multiple echo
void multipleEcho(vector<short>& samples, int sampleRate, int channels, float delaySec, float decay, int repeats) {
    int delaySamples = static_cast<int>(delaySec * sampleRate);
    vector<short> out(samples);

    for (int r = 1; r <= repeats; ++r) {
        for (size_t i = delaySamples * r * channels; i < samples.size(); ++i) {
            int delayed = static_cast<int>(samples[i - delaySamples * r * channels] * pow(decay, r));
            int mixed = static_cast<int>(out[i]) + delayed;
            out[i] = static_cast<short>(max(min(mixed, 32767), -32768));
        }
    }
    samples = out;
}

// Amplitude modulation (tremolo)
void amplitudeModulation(vector<short>& samples, int sampleRate, int channels, float modFreq, float depth) {
    for (size_t i = 0; i < samples.size() / channels; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        float mod = (1.0f - depth) + depth * sin(2.0f * M_PI * modFreq * t);
        for (int c = 0; c < channels; ++c) {
            int idx = i * channels + c;
            int v = static_cast<int>(samples[idx] * mod);
            samples[idx] = static_cast<short>(max(min(v, 32767), -32768));
        }
    }
}

// Time-varying delay (flanger)
void timeVaryingDelay(vector<short>& samples, int sampleRate, int channels, float maxDelaySec, float modFreq) {
    int maxDelaySamples = static_cast<int>(maxDelaySec * sampleRate);
    vector<short> out(samples);

    for (size_t i = 0; i < samples.size() / channels; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        int delaySamples = static_cast<int>((maxDelaySamples / 2.0f) * (1 + sin(2.0f * M_PI * modFreq * t)));

        for (int c = 0; c < channels; ++c) {
            size_t idx = i * channels + c;
            if (i > static_cast<size_t>(delaySamples)) {
                int delayed = samples[(i - delaySamples) * channels + c];
                int mixed = (out[idx] + delayed) / 2;
                out[idx] = static_cast<short>(max(min(mixed, 32767), -32768));
            }
        }
    }
    samples = out;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <input.wav> <output.wav> <effect>\n";
        cerr << "Effects: echo, multi-echo, tremolo, flanger\n";
        return 1;
    }

    const char* inputPath = argv[1];
    const char* outputPath = argv[2];
    string effect = argv[3];

    SndfileHandle inputFile{ inputPath };
    if (inputFile.error()) {
        cerr << "Error: could not open input file.\n";
        return 1;
    }

    if ((inputFile.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV ||
        (inputFile.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
        cerr << "Error: file must be 16-bit PCM WAV.\n";
        return 1;
    }

    int channels = inputFile.channels();
    int sampleRate = inputFile.samplerate();

    vector<short> samples(inputFile.frames() * channels);
    inputFile.readf(samples.data(), inputFile.frames());

    if (effect == "echo") {
        singleEcho(samples, sampleRate, channels, 0.3f, 0.6f);
    } else if (effect == "multi-echo") {
        multipleEcho(samples, sampleRate, channels, 0.25f, 0.5f, 3);
    } else if (effect == "tremolo") {
        amplitudeModulation(samples, sampleRate, channels, 5.0f, 0.7f); // 5 Hz modulation
    } else if (effect == "flanger") {
        timeVaryingDelay(samples, sampleRate, channels, 0.005f, 0.25f); // 5ms delay, 0.25 Hz modulation
    } else {
        cerr << "Unknown effect.\n";
        return 1;
    }

    SndfileHandle outputFile{
        outputPath,
        SFM_WRITE,
        inputFile.format(),
        channels,
        sampleRate
    };

    outputFile.writef(samples.data(), samples.size() / channels);
    cout << "Effect applied successfully: " << effect << "\n";

    return 0;
}

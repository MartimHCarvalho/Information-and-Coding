#include <iostream>
#include <vector>
#include <sndfile.hh>
#include "wav_effects.h"

using namespace std;

int main(int argc, char *argv[])
{
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <input.wav> <output.wav> <effectName>\n";
        cerr << "Effects: none, singleEcho, multipleEcho, amplitudeModulation, timeVaryingDelay, bassBoosted\n";
        return 1;
    }

    const char* inPath = argv[1];
    const char* outPath = argv[2];
    const char* effectName = argv[3];

    SndfileHandle sndFile{inPath};
    if (sndFile.error()) {
        cerr << "Error: invalid input file\n";
        return 1;
    }
    if ((sndFile.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV) {
        cerr << "Error: file is not in WAV format\n";
        return 1;
    }
    if ((sndFile.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
        cerr << "Error: file is not in PCM_16 format\n";
        return 1;
    }

    const int channels = sndFile.channels();
    const int sampleRate = sndFile.samplerate();
    const sf_count_t totalFrames = sndFile.frames();

    vector<short> samples(static_cast<size_t>(totalFrames) * static_cast<size_t>(channels));
    sf_count_t readFrames = sndFile.readf(samples.data(), totalFrames);
    if (readFrames <= 0) {
        cerr << "Error: could not read audio frames\n";
        return 1;
    }
    samples.resize(static_cast<size_t>(readFrames) * static_cast<size_t>(channels));

    WAVEffects fx;
    fx.applyEffect(samples, sampleRate, channels, string(effectName));

    SndfileHandle sfhOut{outPath, SFM_WRITE, sndFile.format(), channels, sampleRate};
    if (sfhOut.error()) {
        cerr << "Error: invalid output file\n";
        return 1;
    }

    if (!fx.to_Wav(sfhOut)) {
        return 1;
    }
    return 0;
}

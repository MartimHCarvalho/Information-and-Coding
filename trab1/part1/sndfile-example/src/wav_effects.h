#ifndef WAV_EFFECTS_H
#define WAV_EFFECTS_H

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sndfile.hh>

class WAVEffects
{
private:
    using Samples = std::vector<short>;

    Samples effect_samples;
    std::map<std::string, std::function<void(Samples&, int, int)>> effects;
    int last_channels = 0;

    static inline short clamp16(int v)
    {
        return static_cast<short>(std::clamp(v, -32768, 32767));
    }

    static constexpr double PI = 3.14159265358979323846;

    void registerEffects()
    {
        effects["none"] = [](Samples& /*s*/, int /*sr*/, int /*ch*/) {};
        effects["singleEcho"] = [this](Samples& s, int sr, int ch) {
            this->applySingleEcho(s, sr, ch, 0.45f, 0.7f);
        };
        effects["multipleEcho"] = [this](Samples& s, int sr, int ch) {
            this->applyMultipleEcho(s, sr, ch, 0.3f, 0.55f, 5);
        };
        effects["amplitudeModulation"] = [this](Samples& s, int sr, int ch) {
            this->applyAmplitudeModulation(s, sr, ch, 5.0f, 0.6f);
        };
        effects["timeVaryingDelay"] = [this](Samples& s, int sr, int ch) {
            this->applyTimeVaryingDelay(s, sr, ch, 0.005f, 0.25f);
        };
        effects["bassBoosted"] = [this](Samples& s, int sr, int ch) {
            this->applyBassBoosted(s, sr, ch, 200.0f, 1.8f);
        };
    }

    void applySingleEcho(Samples& samples, int sampleRate, int channels, float delaySec, float decay)
    {
        if (channels <= 0 || samples.empty()) {
            return;
        }

        const size_t stride = static_cast<size_t>(channels);
        const size_t delayFrames = static_cast<size_t>(std::max(delaySec, 0.0f) * sampleRate);
        if (delayFrames == 0) {
            return;
        }
        const size_t delayOffset = delayFrames * stride;

        Samples out(samples.size(), 0);
        for (size_t i = 0; i < samples.size(); ++i) {
            const double dry = static_cast<double>(samples[i]) * 0.5;
            double wet = 0.0;
            if (i >= delayOffset) {
                wet = static_cast<double>(samples[i - delayOffset]) * decay * 0.5;
            }
            out[i] = clamp16(static_cast<int>(std::lround(dry + wet)));
        }
        samples.swap(out);
    }

    void applyMultipleEcho(Samples& samples, int sampleRate, int channels,
                           float delaySec, float decay, int repeats)
    {
        if (channels <= 0 || samples.empty() || repeats <= 0) {
            return;
        }

        const size_t stride = static_cast<size_t>(channels);
        const size_t delayFrames = static_cast<size_t>(std::max(delaySec, 0.0f) * sampleRate);
        if (delayFrames == 0) {
            return;
        }
        const size_t delayOffset = delayFrames * stride;

        Samples out(samples.size(), 0);
        constexpr double dryMix = 0.35;
        constexpr double wetMix = 0.65;

        for (size_t i = 0; i < samples.size(); ++i) {
            double acc = static_cast<double>(samples[i]) * dryMix;

            for (int r = 1; r <= repeats; ++r) {
                const size_t offset = delayOffset * static_cast<size_t>(r);
                if (i >= offset) {
                    const double weight = wetMix * std::pow(decay, static_cast<double>(r - 1));
                    acc += weight * static_cast<double>(out[i - offset]);
                }
            }

            out[i] = clamp16(static_cast<int>(std::lround(acc)));
        }
        samples.swap(out);
    }

    void applyAmplitudeModulation(Samples& samples, int sampleRate, int channels,
                                  float modFreq, float depth)
    {
        if (channels <= 0 || samples.empty()) {
            return;
        }

        depth = std::clamp(depth, 0.0f, 1.0f);
        const double angularFreq = 2.0 * PI * static_cast<double>(modFreq);

        for (size_t i = 0; i < samples.size(); ++i) {
            const size_t frameIdx = i / static_cast<size_t>(channels);
            const double t = static_cast<double>(frameIdx) / static_cast<double>(sampleRate);

            const double lfo = (std::sin(angularFreq * t) + 1.0) * 0.5;
            const double modFactor = (1.0 - depth) + depth * lfo;

            const int modSample = static_cast<int>(std::lround(static_cast<double>(samples[i]) * modFactor));
            samples[i] = clamp16(modSample);
        }
    }

    void applyTimeVaryingDelay(Samples& samples, int sampleRate, int channels,
                               float baseDelaySec, float modulationFreqHz)
    {
        if (channels <= 0 || samples.empty()) {
            return;
        }

        Samples out(samples.size(), 0);
        const double baseDelaySamples = std::max(0.0, static_cast<double>(baseDelaySec) * sampleRate);
        const double sweepSamples = baseDelaySamples * 0.5;
        const double omega = 2.0 * PI * static_cast<double>(std::max(0.0f, modulationFreqHz));

        for (size_t idx = 0; idx < samples.size(); ++idx) {
            const size_t frameIdx = idx / static_cast<size_t>(channels);
            const size_t channelIdx = idx % static_cast<size_t>(channels);
            const double t = static_cast<double>(frameIdx) / static_cast<double>(sampleRate);

            double currentDelay = baseDelaySamples + sweepSamples * std::sin(omega * t);
            currentDelay = std::clamp(currentDelay, 0.0, baseDelaySamples + sweepSamples);

            const double delayedFrame = static_cast<double>(frameIdx) - currentDelay;
            double wet = 0.0;

            if (delayedFrame >= 0.0) {
                const double baseFrame = std::floor(delayedFrame);
                const double frac = delayedFrame - baseFrame;
                const size_t baseIndex = static_cast<size_t>(baseFrame) * static_cast<size_t>(channels) + channelIdx;

                if (baseIndex < samples.size()) {
                    const short s0 = samples[baseIndex];
                    const short s1 = (baseIndex + channels < samples.size()) ? samples[baseIndex + channels] : s0;
                    wet = (1.0 - frac) * s0 + frac * s1;
                }
            }

            const double dry = samples[idx];
            const int mixed = static_cast<int>(std::lround(0.6 * dry + 0.4 * wet));
            out[idx] = clamp16(mixed);
        }

        samples.swap(out);
    }


    void applyBassBoosted(Samples& samples, int sampleRate, int channels, float cutoffHz, float boostAmount)
    {
        if (channels <= 0 || samples.empty()) return;
        const float RC = 1.0f / (2.0f * PI * cutoffHz);
        const float dt = 1.0f / static_cast<float>(sampleRate);
        const float alpha = dt / (RC + dt);
        
        // For each channel, process separately
        for (int ch = 0; ch < channels; ++ch) {
            float prevOut = 0.0f;
            for (size_t i = ch; i < samples.size(); i += channels) {
                float in = static_cast<float>(samples[i]);
                prevOut = prevOut + alpha * (in - prevOut); // Low-pass filter
                float boosted = in + boostAmount * (prevOut);
                samples[i] = clamp16(static_cast<int>(std::lround(boosted)));
            }
        }
    }



public:
    WAVEffects()
    {
        effect_samples.clear();
        registerEffects();
    }

    void effect(const std::vector<short>& samples, size_t /*num_bits_to_cut*/)
    {
        effect_samples = samples;
    }

    void applyEffect(const std::vector<short>& in, int sampleRate, int channels, const std::string& name)
    {
        if (channels <= 0) {
            effect_samples.clear();
            last_channels = 0;
            return;
        }

        last_channels = channels;
        effect_samples = in;

        auto it = effects.find(name);
        if (it == effects.end()) {
            effects.at("none")(effect_samples, sampleRate, channels);
            return;
        }
        it->second(effect_samples, sampleRate, channels);
    }

    bool to_Wav(SndfileHandle& sfhOut)
    {
        if (effect_samples.empty()) {
            std::cerr << "Error: no samples to write\n";
            return false;
        }
        if (last_channels <= 0) {
            std::cerr << "Error: invalid channel count for output\n";
            return false;
        }

        const size_t channelCount = static_cast<size_t>(last_channels);
        if (effect_samples.size() % channelCount != 0) {
            std::cerr << "Error: sample buffer misaligned with channel count\n";
            return false;
        }

        const sf_count_t frames = static_cast<sf_count_t>(effect_samples.size() / channelCount);
        const sf_count_t written = sfhOut.writef(effect_samples.data(), frames);
        if (written != frames) {
            std::cerr << "Error: failed to write processed audio (" << written << '/' << frames << " frames)\n";
            return false;
        }
        return true;
    }
};

#endif 
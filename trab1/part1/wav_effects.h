#ifndef WAV_EFFECTS_H
#define WAV_EFFECTS_H

#include <vector>

// Apply a single echo (delay + decay)
void singleEcho(std::vector<short>& samples,
                int sampleRate,
                int channels,
                float delaySec,
                float decay);

// Apply multiple echoes (repeated delays with decaying amplitude)
void multipleEcho(std::vector<short>& samples,
                  int sampleRate,
                  int channels,
                  float delaySec,
                  float decay,
                  int repeats);

// Apply amplitude modulation (tremolo effect)
void amplitudeModulation(std::vector<short>& samples,
                         int sampleRate,
                         int channels,
                         float modFreq,
                         float depth);

// Apply time-varying delay (flanger/chorus style)
void timeVaryingDelay(std::vector<short>& samples,
                      int sampleRate,
                      int channels,
                      float maxDelaySec,
                      float modFreq);

#endif 

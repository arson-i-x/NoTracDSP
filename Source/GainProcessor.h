#pragma once

#include <atomic>

#include <juce_audio_basics/juce_audio_basics.h>

// TODO: delay processor
// TODO: audio routing graph
// TODO: plugin hosting (VST3/LV2)
// TODO: preset system
// TODO: touchscreen UI

class GainProcessor final
{
public:
    void prepare (double newSampleRate, int newBlockSize, int newNumChannels) noexcept;

    void process (juce::AudioBuffer<float>& buffer) noexcept;

    void setGain (float newGain) noexcept;

private:
    double sampleRateHz = 0.0;
    int blockSizeSamples = 0;
    int numChannels = 0;
    std::atomic<float> gain { 1.0f };
};
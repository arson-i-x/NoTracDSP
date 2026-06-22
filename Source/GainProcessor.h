#pragma once

#include <atomic>

#include <juce_audio_basics/juce_audio_basics.h>

#include "AudioProcessorBase.h"

// TODO: audio routing graph
// TODO: plugin hosting (VST3/LV2)
// TODO: preset system
// TODO: touchscreen UI

class GainProcessor final : public AudioProcessorBase
{
public:
    void prepare (double newSampleRate, int newBlockSize, int newNumChannels) noexcept {
        sampleRateHz = newSampleRate;
        blockSizeSamples = newBlockSize;
        numChannels = newNumChannels;
    };

    void process (juce::AudioBuffer<float>& buffer) noexcept { buffer.applyGain (gain.load (std::memory_order_relaxed)); };

    void setGain (float newGain) noexcept { gain.store (newGain, std::memory_order_relaxed); };

private:
    double sampleRateHz = 0.0;
    int blockSizeSamples = 0;
    int numChannels = 0;
    std::atomic<float> gain { 1.0f };
};
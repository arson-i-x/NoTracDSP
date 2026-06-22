#pragma once

#include <atomic>

#include <juce_audio_basics/juce_audio_basics.h>

#include "AudioProcessorBase.h"

class DelayProcessor final : public AudioProcessorBase
{
public:
    void prepare (double newSampleRate, int newBlockSize, int newNumChannels) noexcept;

    void process (juce::AudioBuffer<float>& buffer) noexcept;

    void setDelayTimeMs(float ms) noexcept { delayMs.store(ms, std::memory_order_relaxed); }

    void setDelayMix(float mix) noexcept { wetMix.store(mix, std::memory_order_relaxed); }

private:
    juce::AudioBuffer<float> delayBuffer;
    int writePosition = 0;
    double sampleRateHz = 0.0;
    int numChannels = 0;
    int blockSizeSamples = 0;
    std::atomic<float> wetMix = { 0.5f }; // 50% wet, 50% dry mix
    std::atomic<float> delayMs { 1000.0f };
};
#pragma once

#include <atomic>

#include <juce_audio_basics/juce_audio_basics.h>

class DelayProcessor final
{
public:
    void prepare (double newSampleRate, int newBlockSize, int newNumChannels) noexcept;

    void process (juce::AudioBuffer<float>& buffer) noexcept;

    void setDelayTimeMs(float ms);

private:
    juce::AudioBuffer<float> delayBuffer;
    int writePosition = 0;
    double sampleRateHz = 0.0;
    int numChannels = 0;
    int blockSizeSamples = 0;
    std::atomic<float> delayMs { 1000.0f };
};
#include "GainProcessor.h"

void GainProcessor::prepare (double newSampleRate, int newBlockSize, int newNumChannels) noexcept
{
    sampleRateHz = newSampleRate;
    blockSizeSamples = newBlockSize;
    numChannels = newNumChannels;
}

void GainProcessor::process (juce::AudioBuffer<float>& buffer) noexcept
{
    // Real-time audio thread: keep this branch-free, lock-free, and allocation-free.
    buffer.applyGain (gain.load (std::memory_order_relaxed));
}

void GainProcessor::setGain (float newGain) noexcept
{
    gain.store (newGain, std::memory_order_relaxed);
}
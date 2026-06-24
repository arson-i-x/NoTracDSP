#pragma once

#include <atomic>

#include <juce_audio_basics/juce_audio_basics.h>

class AudioProcessorBase
{
public:

    virtual ~AudioProcessorBase() = default;

    virtual void prepare(
        double sampleRate,
        int blockSize,
        int channels) = 0;

    virtual void process(juce::AudioBuffer<float>& buffer) = 0;
};
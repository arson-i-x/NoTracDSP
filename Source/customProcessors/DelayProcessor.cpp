#include "DelayProcessor.h"

void DelayProcessor::prepare (double newSampleRate, int newBlockSize, int newNumChannels) noexcept
{
    sampleRateHz = newSampleRate;
    blockSizeSamples = newBlockSize;
    numChannels = newNumChannels;

    // TODO: make this user-configurable
    const auto maxDelayTimeSeconds = 2.0; 

    const auto maxDelayTimeSamples = static_cast<int> (maxDelayTimeSeconds * sampleRateHz);
    delayBuffer.setSize (numChannels, maxDelayTimeSamples);
    delayBuffer.clear();
    writePosition = 0;
}

void DelayProcessor::process (juce::AudioBuffer<float>& buffer) noexcept
{
    // Delay time in samples, calculated from the current delay time in milliseconds and the sample rate.
    const auto delaySamples =
        static_cast<int>(
            sampleRateHz * delayMs.load() / 1000.0f);

    const auto mix = wetMix.load();

    if (delaySamples <= 0 || delaySamples >= delayBuffer.getNumSamples())
        return; // Invalid delay time, do nothing.

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            // get pointers to the current channel data in the input buffer and the delay buffer
            auto* channelData = buffer.getWritePointer (channel);
            auto* delayData = delayBuffer.getWritePointer (channel);

            // calculate the read position in the delay buffer, wrapping around if necessary
            int readPosition =
                (writePosition
                - delaySamples
                + delayBuffer.getNumSamples())
                % delayBuffer.getNumSamples();

            // read the delayed sample from the delay buffer at the calculated read position
            const auto delayedSample = delayData[readPosition];

            // read current input sample from the input buffer
            const auto inputSample = channelData[sample];

            // output delayed sample to output buffer
            channelData[sample] = delayedSample * mix + inputSample * (1.0f - mix); // mix delayed sample with current input sample

            // write current input sample to delay buffer at current write position
            delayData[writePosition] = inputSample;
        }
        // advance write position, wrapping around to the beginning of the buffer if necessary
        if (++writePosition >= delayBuffer.getNumSamples()) writePosition = 0;
    }    
}  
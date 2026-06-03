#include <DelayProcessor.h>

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


    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            auto* delayData = delayBuffer.getWritePointer (channel);

            // calculate the read position for the delayed sample 
            int readPosition = writePosition - delaySamples;

            // wrap around the read position if it's negative, so it correctly reads from the end of the delay buffer
            if (readPosition < 0)
                readPosition += delayBuffer.getNumSamples();

            // read the delayed sample from the delay buffer at the calculated read position
            const auto delayedSample = delayData[readPosition];

            // read current input sample from the input buffer
            const auto inputSample = channelData[sample];

            // output delayed sample to output buffer
            channelData[sample] = delayedSample;

            // write current input sample to delay buffer at current write position
            delayData[writePosition] = inputSample;
        }
        // advance write position, wrapping around to the beginning of the buffer if necessary
        if (++writePosition >= delayBuffer.getNumSamples()) writePosition = 0;
    }    
}  

void DelayProcessor::setDelayTimeMs(float ms)
{
    delayMs.store(ms, std::memory_order_relaxed);
}
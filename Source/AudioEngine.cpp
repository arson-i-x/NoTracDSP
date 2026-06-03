#include "AudioEngine.h"

AudioEngine::AudioEngine()
{
    const auto initResult = audioDeviceManager.initialise (2, 2, nullptr, true);

    if (initResult.isNotEmpty())
        deviceStatus.statusText = initResult;

    // GUI thread: listen for device changes so the UI can refresh status labels safely.
    audioDeviceManager.addChangeListener (this);
    audioDeviceManager.addAudioCallback (this);
    refreshDeviceStatus();
}

AudioEngine::~AudioEngine()
{
    audioDeviceManager.removeAudioCallback (this);
    audioDeviceManager.removeChangeListener (this);
}

AudioEngine::DeviceStatus AudioEngine::getDeviceStatus() const noexcept
{
    return deviceStatus;
}

void AudioEngine::setMasterGain (float newGain) noexcept
{
    gainProcessor.setGain (newGain);
}

void AudioEngine::addStatusListener (juce::ChangeListener* listener)
{
    addChangeListener (listener);
}

void AudioEngine::removeStatusListener (juce::ChangeListener* listener)
{
    removeChangeListener (listener);
}

void AudioEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    const auto sampleRate = device != nullptr ? device->getCurrentSampleRate() : 0.0;
    const auto blockSize = device != nullptr ? device->getCurrentBufferSizeSamples() : 0;
    const auto numChannels = device != nullptr ? juce::jmax (1, device->getActiveOutputChannels().countNumberOfSetBits())
                                               : 1;

    gainProcessor.prepare (sampleRate, blockSize, numChannels);
    delayProcessor.prepare (sampleRate, blockSize, numChannels);
    refreshDeviceStatus();
}

void AudioEngine::audioDeviceStopped()
{
    gainProcessor.prepare (0.0, 0, 0);
    delayProcessor.prepare (0.0, 0, 0);
}

void AudioEngine::audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                                    int numInputChannels,
                                                    float* const* outputChannelData,
                                                    int numOutputChannels,
                                                    int numSamples,
                                                    const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused (context);

    // Real-time audio thread: no locks, no allocations, no blocking work.
    const auto channelsToCopy = juce::jmin (numInputChannels, numOutputChannels);

    // Copy input channels to output channels, clearing any output channels that don't have an input.
    for (int channel = 0; channel < channelsToCopy; ++channel)
    {
        auto* output = outputChannelData[channel];
        const auto* input = inputChannelData[channel];

        // If the output is null, we have nowhere to copy the input data to, so skip this channel.
        if (output == nullptr)
            continue;

        // If the input is null, we have no data to copy, so clear the output buffer to avoid noise.
        if (input != nullptr)
            juce::FloatVectorOperations::copy (output, input, numSamples);
        else
            juce::FloatVectorOperations::clear (output, numSamples);
    }

    // Fill any remaining output channels with copies of the first input channel (or silence if no inputs are active).
    for (int channel = channelsToCopy; channel < numOutputChannels; ++channel)
    {
        auto* output = outputChannelData[channel];

        // If the output is null, we have nowhere to copy the input data to, so skip this channel.
        if (output == nullptr)
            continue;

        // If we have at least one input channel with data, copy the first input channel to the remaining output channels. 
        if (numInputChannels > 0 && inputChannelData[0] != nullptr)
            juce::FloatVectorOperations::copy (output, inputChannelData[0], numSamples);
        // Otherwise, clear the output buffer to avoid noise.
        else
            juce::FloatVectorOperations::clear (output, numSamples);
    }

    // Create a non-const pointer to the output channel data so we can wrap it in an AudioBuffer without copying.
    auto** mutableOutputData = const_cast<float**> (outputChannelData);

    // This creates an AudioBuffer that wraps the output channel data without copying it, allowing us to apply the gain processor directly to the output buffer.
    juce::AudioBuffer<float> outputBuffer { mutableOutputData, numOutputChannels, numSamples };

    // Apply the gain processor to the output buffer in-place.
    gainProcessor.process (outputBuffer);

    // Apply the delay processor to the output buffer in-place.
    delayProcessor.process (outputBuffer);
}

void AudioEngine::changeListenerCallback (juce::ChangeBroadcaster*)
{
    triggerAsyncUpdate();
}

void AudioEngine::handleAsyncUpdate()
{
    refreshDeviceStatus();
    sendChangeMessage();
}

void AudioEngine::refreshDeviceStatus() noexcept
{
    auto newStatus = DeviceStatus{};

    if (auto* deviceType = audioDeviceManager.getCurrentDeviceTypeObject())
        newStatus.deviceType = deviceType->getTypeName();

    if (auto* device = audioDeviceManager.getCurrentAudioDevice())
    {
        newStatus.deviceName = device->getName();
        newStatus.formatText = "Sample rate: " + juce::String (device->getCurrentSampleRate(), 1)
                             + " Hz | Buffer size: " + juce::String (device->getCurrentBufferSizeSamples()) + " samples";
        newStatus.channelText = "Input: " + device->getInputChannelNames().joinIntoString (", ")
                              + " | Output: " + device->getOutputChannelNames().joinIntoString (", ");
        newStatus.latencyText = "Input latency: " + juce::String (device->getInputLatencyInSamples())
                              + " samples | Output latency: " + juce::String (device->getOutputLatencyInSamples()) + " samples";
        newStatus.statusText = "Device active";
    }
    else
    {
        newStatus.deviceName = "No audio device open";
        newStatus.formatText = "Sample rate: - | Buffer size: -";
        newStatus.channelText = "Input: - | Output: -";
        newStatus.latencyText = "Input latency: - | Output latency: -";

        if (deviceStatus.statusText.isEmpty())
            newStatus.statusText = "No audio device open";
        else
            newStatus.statusText = deviceStatus.statusText;
    }

    deviceStatus = newStatus;
}
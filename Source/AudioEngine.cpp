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

void AudioEngine::setMasterGain (float newGain) noexcept
{
    gainProcessor.setGain (newGain);
}

AudioEngine::DeviceStatus AudioEngine::getDeviceStatus() const noexcept
{
    return deviceStatus;
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
    refreshDeviceStatus();
}

void AudioEngine::audioDeviceStopped()
{
    gainProcessor.prepare (0.0, 0, 0);
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

    for (int channel = 0; channel < channelsToCopy; ++channel)
    {
        auto* output = outputChannelData[channel];
        const auto* input = inputChannelData[channel];

        if (output == nullptr)
            continue;

        if (input != nullptr)
            juce::FloatVectorOperations::copy (output, input, numSamples);
        else
            juce::FloatVectorOperations::clear (output, numSamples);
    }

    for (int channel = channelsToCopy; channel < numOutputChannels; ++channel)
    {
        auto* output = outputChannelData[channel];

        if (output == nullptr)
            continue;

        if (numInputChannels > 0 && inputChannelData[0] != nullptr)
            juce::FloatVectorOperations::copy (output, inputChannelData[0], numSamples);
        else
            juce::FloatVectorOperations::clear (output, numSamples);
    }

    auto** mutableOutputData = const_cast<float**> (outputChannelData);
    juce::AudioBuffer<float> outputBuffer { mutableOutputData, numOutputChannels, numSamples };
    gainProcessor.process (outputBuffer);
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
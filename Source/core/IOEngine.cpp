#include "IOEngine.h"

IOEngine::IOEngine(ProcessingEngine& processingEngine) : juce::AudioIODeviceCallback(),
                                                        juce::ChangeListener(),
                                                        juce::AsyncUpdater(),
                                                        juce::ChangeBroadcaster(),
                                                        processingEngine(processingEngine)
{
    const auto initResult = audioDeviceManager.initialise (1, 2, nullptr, true);

    if (initResult.isNotEmpty())
        deviceStatus.statusText = initResult;

    // GUI thread: listen for device changes so the UI can refresh status labels safely.
    audioDeviceManager.addChangeListener (this);
    audioDeviceManager.addAudioCallback (this);
    refreshDeviceStatus();
}

void IOEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    if (!device)
    {
        AppMessageBus::getInstance().error ("Audio device is null.", "Please check your audio device settings.");
        return;
    }

    sampleRate = device->getCurrentSampleRate();
    blockSize = device->getCurrentBufferSizeSamples();
    numChannels = device->getActiveOutputChannels().countNumberOfSetBits();

    graphBuffer.setSize(2, blockSize);
    graphBuffer.clear();

    sendChangeMessage();
}

void IOEngine::audioDeviceStopped()
{
    sampleRate = 0.0;
    blockSize = 0;
    numChannels = 0;
    graphBuffer.setSize(0, 0);
    graphBuffer.clear();
}

void IOEngine::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(context);

    for (int ch = 0; ch < numOutputChannels; ++ch)
        if (outputChannelData[ch] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);

    graphBuffer.clear();

    if (numInputChannels > 0 && inputChannelData[0] != nullptr)
    {
        // Clarett input 1 -> graph stereo input
        graphBuffer.copyFrom(0, 0, inputChannelData[0], numSamples);
        graphBuffer.copyFrom(1, 0, inputChannelData[0], numSamples);
    }

    midiBuffer.clear();

    processingEngine.process(graphBuffer, midiBuffer);

    if (numOutputChannels > 0 && outputChannelData[0] != nullptr)
        juce::FloatVectorOperations::copy(
            outputChannelData[0],
            graphBuffer.getReadPointer(0),
            numSamples);

    if (numOutputChannels > 1 && outputChannelData[1] != nullptr)
        juce::FloatVectorOperations::copy(
            outputChannelData[1],
            graphBuffer.getReadPointer(1),
            numSamples);
}

void IOEngine::changeListenerCallback (juce::ChangeBroadcaster*)
{
    triggerAsyncUpdate();
}

void IOEngine::handleAsyncUpdate()
{
    refreshDeviceStatus();
    sendChangeMessage();
}

void IOEngine::refreshDeviceStatus() noexcept
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


void IOEngine::shutdown()
{
    audioDeviceManager.removeAudioCallback(this);
    audioDeviceManager.removeChangeListener(this);
}

DeviceStatus IOEngine::getDeviceStatus() const noexcept
{
    return deviceStatus;
}
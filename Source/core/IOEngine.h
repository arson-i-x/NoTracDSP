#pragma once

#include <atomic>

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "core/ProcessingEngine.h"
#include "core/PluginRegistry.h"
#include "core/PluginGraphModel.h"
#include "core/AppMessageBus.h"
#include "core/Result.h"
#include "core/Status.h"

#include "commands/AppCommand.h"

#include "customProcessors/PolyphonicOctaver.h"

struct DeviceStatus
{
    juce::String deviceType = "No device";
    juce::String deviceName = "No audio device open";
    juce::String formatText = "Sample rate: - | Buffer size: -";
    juce::String channelText = "Input: - | Output: -";
    juce::String latencyText = "Input latency: - | Output latency: -";
    juce::String statusText = "Initializing...";
};

struct PluginSnapshot
{
    juce::PluginDescription desc;
    juce::String name;
    bool bypassed = false;
    int chainIndex = -1;
    juce::MemoryBlock state;
};

class IOEngine final : public juce::AudioIODeviceCallback,
                          public juce::ChangeBroadcaster,
                          private juce::ChangeListener,
                          private juce::AsyncUpdater
{
public:
    IOEngine(ProcessingEngine& engine);
    ~IOEngine() override = default;

    double getSampleRate() const noexcept { return sampleRate; }
    int getBlockSize() const noexcept { return blockSize; }
    int getNumChannels() const noexcept { return numChannels; }

    [[nodiscard]] DeviceStatus getDeviceStatus() const noexcept;

    void audioDeviceAboutToStart(juce::AudioIODevice *device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(const float *const *inputChannelData,
                                          int numInputChannels,
                                          float *const *outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext &context) override;

    void shutdown();

    juce::AudioDeviceManager &getAudioDeviceManager() noexcept { return audioDeviceManager; }
    const juce::AudioDeviceManager &getAudioDeviceManager() const noexcept { return audioDeviceManager; }

private:
    ProcessingEngine& processingEngine;

    juce::AudioDeviceManager audioDeviceManager;

    DeviceStatus deviceStatus;

    void changeListenerCallback(juce::ChangeBroadcaster *) override;
    void handleAsyncUpdate() override;
    void refreshDeviceStatus() noexcept;
    void connectStereo(juce::AudioProcessorGraph::NodeID source, juce::AudioProcessorGraph::NodeID dest);

    juce::AudioBuffer<float> graphBuffer;
    juce::MidiBuffer midiBuffer;

    double sampleRate = 0.0;
    int blockSize = 0;
    int numChannels = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IOEngine)
};
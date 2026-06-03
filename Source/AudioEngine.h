#pragma once

#include "GainProcessor.h"
#include "DelayProcessor.h"
#include <atomic>
#include <juce_audio_devices/juce_audio_devices.h>

class AudioEngine final : public juce::AudioIODeviceCallback,
                          private juce::ChangeListener,
                          private juce::AsyncUpdater,
                          private juce::ChangeBroadcaster
{
public:
    struct DeviceStatus
    {
        juce::String deviceType = "No device";
        juce::String deviceName = "No audio device open";
        juce::String formatText = "Sample rate: - | Buffer size: -";
        juce::String channelText = "Input: - | Output: -";
        juce::String latencyText = "Input latency: - | Output latency: -";
        juce::String statusText = "Initializing...";
    };

    AudioEngine();
    ~AudioEngine() override;

    juce::AudioDeviceManager& getAudioDeviceManager() noexcept { return audioDeviceManager; }
    const juce::AudioDeviceManager& getAudioDeviceManager() const noexcept { return audioDeviceManager; }

    void setMasterGain (float newGain) noexcept;

    [[nodiscard]] DeviceStatus getDeviceStatus() const noexcept;

    void addStatusListener (juce::ChangeListener* listener);
    void removeStatusListener (juce::ChangeListener* listener);

    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext& context) override;

private:
    juce::AudioDeviceManager audioDeviceManager;
    GainProcessor gainProcessor;
    DelayProcessor delayProcessor;
    DeviceStatus deviceStatus;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void handleAsyncUpdate() override;
    void refreshDeviceStatus() noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEngine)
};
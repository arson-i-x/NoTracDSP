#pragma once

#include "AudioProcessorBase.h"
#include "GainProcessor.h"
#include "DelayProcessor.h"
#include <atomic>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>

struct ActivePluginInfo
{
    juce::AudioProcessorGraph::NodeID nodeId;
    juce::String name;
    bool bypassed = false;
};

struct ActivePlugin
{
    juce::AudioProcessorGraph::NodeID nodeId;
    juce::PluginDescription desc;
    juce::String name;
    bool bypassed = false;
};

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

    juce::AudioProcessorGraph& getAudioProcessorGraph() noexcept { return audioProcessorGraph; }
    const juce::AudioProcessorGraph& getAudioProcessorGraph() const noexcept { return audioProcessorGraph; }

    juce::AudioProcessor* getProcessorForNode(juce::AudioProcessorGraph::NodeID nodeId);

    double getSampleRate() const noexcept { return sampleRate; }
    int getBlockSize() const noexcept { return blockSize; }
    int getNumChannels() const noexcept { return numChannels; }

    void setMasterGain (float newGain) noexcept;

    void setDelayTimeMs (float ms) noexcept;

    void setDelayMix (float mix) noexcept;

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

    // Adds the plugin and returns a ptr to its node for front end
    juce::AudioProcessorGraph::Node::Ptr addPlugin(
        const juce::PluginDescription& desc,
        juce::AudioPluginFormatManager& formatManager);

    // void swapPlugin(size_t indexA, size_t indexB);
    std::vector<ActivePluginInfo> getActivePlugins() const;

    void removePlugin(juce::AudioProcessorGraph::NodeID nodeId);
    void setPluginOrder(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder);
    void toggleBypass(juce::AudioProcessorGraph::NodeID nodeId);
    void rebuildGraphConnections();
    void setPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId, bool shouldBeBypassed);
    bool isPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId) const;
    juce::String getPluginName(juce::AudioProcessorGraph::NodeID nodeId) const;
    

private:
    juce::AudioProcessorGraph audioProcessorGraph;
    juce::AudioProcessorGraph::Node::Ptr inputNode;
    juce::AudioProcessorGraph::Node::Ptr outputNode;

    std::vector<ActivePlugin> activePlugins;

    juce::AudioDeviceManager audioDeviceManager;
    
    DeviceStatus deviceStatus;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void handleAsyncUpdate() override;
    void refreshDeviceStatus() noexcept;
    void setPluginInstance(std::unique_ptr<juce::AudioPluginInstance> newPlugin);
    void connectStereo(juce::AudioProcessorGraph::NodeID source, juce::AudioProcessorGraph::NodeID dest);

    juce::AudioBuffer<float> graphBuffer;
    juce::MidiBuffer midiBuffer;

    double sampleRate = 0.0;
    int blockSize = 0;
    int numChannels = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEngine)
};
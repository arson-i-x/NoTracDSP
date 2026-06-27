#pragma once

#include <atomic>

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "core/AppMessageBus.h"

#include "commands/AppCommand.h"

#include "customProcessors/AudioProcessorBase.h"
#include "customProcessors/GainProcessor.h"
#include "customProcessors/DelayProcessor.h"

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

    struct PluginSnapshot {
        juce::PluginDescription desc;
        juce::String name;
        bool bypassed = false;
        int chainIndex = -1;
        juce::MemoryBlock state;
    };

    AudioEngine();
    ~AudioEngine() override = default;

    juce::AudioPluginFormatManager& getPluginFormatManager() noexcept { return pluginFormatManager; }

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

    void addStatusListener (juce::ChangeListener* listener) { addChangeListener (listener); };
    void removeStatusListener (juce::ChangeListener* listener) { removeChangeListener (listener); };

    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext& context) override;

    // Adds the plugin and returns a ptr to its node for front end
    AppCommand::Result<juce::AudioProcessorGraph::Node::Ptr> addPlugin(
        const juce::PluginDescription& desc,
        juce::AudioPluginFormatManager& formatManager);

    // void swapPlugin(size_t indexA, size_t indexB);
    std::vector<AudioEngine::ActivePluginInfo> getActivePlugins() const;
    
    AppCommand::Result<PluginSnapshot> getPluginSnapshot(juce::AudioProcessorGraph::NodeID nodeId) const;
    AppCommand::Status removePlugin(juce::AudioProcessorGraph::NodeID nodeId);
    AppCommand::Status togglePluginBypass(juce::AudioProcessorGraph::NodeID nodeId);
    AppCommand::Status setPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId, bool shouldBeBypassed);
    AppCommand::Status setPluginOrder(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder);
    AppCommand::Status restorePluginSnapshot(const PluginSnapshot& snapshot, juce::AudioPluginFormatManager& formatManager);

    AppCommand::Status canFindPlugin(juce::AudioProcessorGraph::NodeID nodeId) const;
    AppCommand::Status canSetPluginOrder(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder) const;
    AppCommand::Status canRestorePluginSnapshot(const PluginSnapshot& snapshot) const;
    AppCommand::Status canAddPlugin(const juce::PluginDescription& desc) const;

    void rebuildGraphConnections();
    bool isPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId) const;
    juce::String getPluginName(juce::AudioProcessorGraph::NodeID nodeId) const;
    
    void setPluginName(juce::AudioProcessorGraph::NodeID nodeId, const juce::String& newName);
    AudioEngine::ActivePluginInfo getActivePluginInfo(juce::AudioProcessorGraph::NodeID nodeId) const;
    AudioEngine::ActivePlugin getActivePlugin(juce::AudioProcessorGraph::NodeID nodeId) const;
    juce::AudioProcessorGraph::Node::Ptr getNodeForId(juce::AudioProcessorGraph::NodeID nodeId) const;

    void shutdown();
private: 
    juce::AudioProcessorGraph audioProcessorGraph;
    juce::AudioProcessorGraph::Node::Ptr inputNode;
    juce::AudioProcessorGraph::Node::Ptr outputNode;

    std::vector<ActivePlugin> activePlugins;

    juce::AudioDeviceManager audioDeviceManager;

    juce::AudioPluginFormatManager pluginFormatManager;
    
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
    bool isStillRegisteredSomewhere = false; // optional flag to track if the singleton is still registered somewhere
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEngine)
};
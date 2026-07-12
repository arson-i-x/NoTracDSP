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

class AudioEngine final : public juce::AudioIODeviceCallback,
                          public juce::ChangeBroadcaster,
                          private juce::ChangeListener,
                          private juce::AsyncUpdater
{
public:
    AudioEngine();
    ~AudioEngine() override = default;

    juce::AudioProcessor *getProcessorForNode(juce::AudioProcessorGraph::NodeID nodeId);

    double getSampleRate() const noexcept { return sampleRate; }
    int getBlockSize() const noexcept { return blockSize; }
    int getNumChannels() const noexcept { return numChannels; }
    const std::vector<juce::AudioProcessorGraph::NodeID> getPluginOrder() const;
    const std::vector<ActivePlugin>& getActivePluginsInfo() const noexcept;

    void setMasterGain(float newGain) noexcept;

    [[nodiscard]] DeviceStatus getDeviceStatus() const noexcept;

    void addStatusListener(juce::ChangeListener *listener) 
    { juce::MessageManager::callAsync([this, listener]() { addChangeListener(listener); }); };
    void removeStatusListener(juce::ChangeListener *listener) 
    { juce::MessageManager::callAsync([this, listener]() { removeChangeListener(listener); }); };

    void audioDeviceAboutToStart(juce::AudioIODevice *device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(const float *const *inputChannelData,
                                          int numInputChannels,
                                          float *const *outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext &context) override;

    juce::ValueTree createPresetState() const;

    // Adds the plugin and returns a ptr to its node for front end
    Result<juce::AudioProcessorGraph::Node::Ptr> addPlugin(const juce::PluginDescription &desc);

    std::optional<juce::PluginDescription> findPluginByIdentifier(
        const juce::KnownPluginList &knownPlugins, const juce::String &identifier) const;

    Status restorePresetState(const juce::ValueTree& preset);
    Result<PluginSnapshot> getPluginSnapshot(juce::AudioProcessorGraph::NodeID nodeId) const;
    Status removePlugin(juce::AudioProcessorGraph::NodeID nodeId);
    Status togglePluginBypass(juce::AudioProcessorGraph::NodeID nodeId);
    Status setPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId, bool shouldBeBypassed);
    Status setPluginOrder(const std::vector<juce::AudioProcessorGraph::NodeID> &newOrder);
    Result<juce::AudioProcessorGraph::Node::Ptr> restorePluginSnapshot(const PluginSnapshot &snapshot);

    Status canFindPlugin(juce::AudioProcessorGraph::NodeID nodeId) const;
    Status canSetPluginOrder(const std::vector<juce::AudioProcessorGraph::NodeID> &newOrder) const;
    Status canAddPlugin(const juce::PluginDescription &desc) const;
    Status canRestorePluginSnapshot(const PluginSnapshot &snapshot) const;

    void rebuildGraphConnections();
    bool isPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId) const;
    juce::String getPluginName(juce::AudioProcessorGraph::NodeID nodeId) const;

    void setPluginName(juce::AudioProcessorGraph::NodeID nodeId, const juce::String &newName);
    ActivePlugin getActivePlugin(juce::AudioProcessorGraph::NodeID nodeId) const;
    juce::AudioProcessorGraph::Node::Ptr getNodeForId(juce::AudioProcessorGraph::NodeID nodeId) const;

    void clearPlugins();

    void shutdown();

    juce::AudioDeviceManager &getAudioDeviceManager() noexcept { return audioDeviceManager; }
    const juce::AudioDeviceManager &getAudioDeviceManager() const noexcept { return audioDeviceManager; }

    juce::AudioProcessorGraph &getAudioProcessorGraph() noexcept { return processingEngine->getGraph(); }
    const juce::AudioProcessorGraph &getAudioProcessorGraph() const noexcept { return processingEngine->getGraph(); }

    PluginRegistry &getPluginRegistry() noexcept { return pluginRegistry; }
    const PluginRegistry &getPluginRegistry() const noexcept { return pluginRegistry; }

    juce::KnownPluginList &getKnownPluginList() noexcept { return pluginRegistry.getKnownPluginList(); }
    const juce::KnownPluginList &getKnownPluginList() const noexcept { return pluginRegistry.getKnownPluginList(); }

    PluginGraphModel &getPluginGraphModel() noexcept { return pluginGraphModel; }
    const PluginGraphModel &getPluginGraphModel() const noexcept { return pluginGraphModel; }

private:
    std::unique_ptr<ProcessingEngine> processingEngine;

    PluginRegistry pluginRegistry;

    PluginGraphModel pluginGraphModel;

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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
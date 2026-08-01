#pragma once

#include <optional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

struct ActivePlugin
{
    juce::PluginDescription desc;
    juce::String displayName = "Unknown Plugin";
    bool bypassed = false;
    int chainIndex = -1;
    juce::AudioProcessorGraph::NodeID nodeId { 0 };
    std::optional<juce::MemoryBlock> state = std::nullopt;
};

class PluginGraphModel : public juce::ChangeBroadcaster
{
private:
    std::vector<ActivePlugin> activePlugins;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginGraphModel)
public:
    PluginGraphModel() = default;

    bool addActivePlugin(const ActivePlugin& plugin, juce::AudioProcessorGraph::NodeID nodeId);
    bool addPluginDescription(const juce::PluginDescription& desc, juce::AudioProcessorGraph::NodeID nodeId);

    bool isEmpty() const;
    void setPluginState(juce::AudioProcessorGraph::NodeID nodeId, ActivePlugin plugin);
    juce::String getDisplayName(juce::AudioProcessorGraph::NodeID nodeId) const;
    void clearPlugins();

    bool removePlugin(juce::AudioProcessorGraph::NodeID nodeId);
    void setPluginOrder(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder);
    bool setPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId, bool bypassed);
    bool toggleBypass(juce::AudioProcessorGraph::NodeID nodeId);
    bool validOrder(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder) const;

    // gives a list of active plugin structs in the order they were added to the graph
    const std::vector<ActivePlugin>& getActivePlugins() const;
    const ActivePlugin& getActivePlugin(juce::AudioProcessorGraph::NodeID nodeId) const;
};
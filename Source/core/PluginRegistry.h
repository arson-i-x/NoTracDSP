#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PluginRegistry
{
private:
    juce::KnownPluginList knownPluginList;
    juce::AudioPluginFormatManager pluginFormatManager;
    std::vector<ActivePlugin> activePlugins;

public:
    PluginRegistry();

    Result<juce::AudioProcessorGraph::Node::Ptr> addPlugin(
    const juce::PluginDescription& desc)
    {
        juce::String error;

        auto plugin = pluginFormatManager.createPluginInstance(
            desc,
            sampleRate,
            blockSize,
            error);

        if (!plugin)
        {
            DBG("Failed to load plugin: " + error);
            return Result<juce::AudioProcessorGraph::Node::Ptr>::failure("Failed to load plugin: " + error);
        }

        auto node = audioProcessorGraph.addNode(std::move(plugin));
        
        if (!node)
        {
            DBG("Failed to add plugin node to graph.");
            return Result<juce::AudioProcessorGraph::Node::Ptr>
                            ::failure("Failed to add plugin node to graph.");
        }

        activePlugins.push_back({
            node->nodeID,
            desc,
            desc.name,
            node->isBypassed(),
            static_cast<int>(activePlugins.size())
        });

        rebuildGraphConnections();

        return Result<juce::AudioProcessorGraph::Node::Ptr>
                        ::success(node);
    }


    juce::KnownPluginList& getKnownPluginList() noexcept { return knownPluginList; }
    const juce::KnownPluginList& getKnownPluginList() const noexcept { return knownPluginList; }

    juce::AudioPluginFormatManager& getPluginFormatManager() noexcept { return pluginFormatManager; }
    const juce::AudioPluginFormatManager& getPluginFormatManager() const noexcept { return pluginFormatManager; }
};
    
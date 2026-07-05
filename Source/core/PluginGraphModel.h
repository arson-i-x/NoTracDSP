#pragma once

#include <algorithm>
#include <juce_audio_processors/juce_audio_processors.h>

struct ActivePlugin
{
    juce::AudioProcessorGraph::NodeID nodeId;
    juce::PluginDescription desc;
    juce::String displayName;
    bool bypassed = false;
    int chainIndex = -1;
};

class PluginGraphModel
{
private:
    std::vector<ActivePlugin> activePlugins;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginGraphModel)
public:
    PluginGraphModel() = default;

    bool addPlugin(const ActivePlugin& plugin)
    {
        if (plugin.desc.name.isEmpty() || plugin.nodeId.uid == 0)
            return false;

        activePlugins.push_back(plugin);
        return true;
    }

    bool addPlugin(const juce::PluginDescription& desc, juce::AudioProcessorGraph::NodeID nodeId)
    {
        if (desc.name.isEmpty())
            return false;

        if (nodeId.uid == 0)
            return false;

        ActivePlugin plugin;
        plugin.desc = desc;
        plugin.displayName = desc.name;
        plugin.bypassed = false;
        plugin.nodeId = nodeId;

        activePlugins.push_back(plugin);
        return true;
    }

    void clearPlugins()
    {
        activePlugins.clear();
    }

    std::vector<ActivePlugin>& getActivePlugins()
    {
        return activePlugins;
    }

    bool removePlugin(juce::AudioProcessorGraph::NodeID nodeId)
    {
        for (auto it = activePlugins.begin(); it != activePlugins.end(); ++it)
        {
            if (it->nodeId == nodeId)
            {
                activePlugins.erase(it);
                return true;
            }
        }

        return false;
    }

    bool setPluginOrder(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder)
    {
        if (newOrder.size() != activePlugins.size())
            return false;

        std::vector<ActivePlugin> reordered;
        reordered.reserve(activePlugins.size());

        for (const auto nodeId : newOrder)
        {
            auto it = std::find_if(activePlugins.begin(), activePlugins.end(),
                [nodeId](const ActivePlugin& plugin)
                {
                    return plugin.nodeId == nodeId;
                });

            if (it == activePlugins.end())
                return false;

            reordered.push_back(*it);
        }

        activePlugins = std::move(reordered);
        for (int i = 0; i < (int) activePlugins.size(); ++i)
            activePlugins[i].chainIndex = i;

        return true;
    }

    bool isPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId) const
    {
        for (const auto& plugin : activePlugins)
        {
            if (plugin.nodeId == nodeId)
                return plugin.bypassed;
        }

        return false;
    }

    bool setPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId, bool bypassed)
    {
        for (auto& plugin : activePlugins)
        {
            if (plugin.nodeId == nodeId)
            {
                plugin.bypassed = bypassed;
                return true;
            }
        }

        return false;
    }

    const std::vector<ActivePlugin>& getActivePlugins() const
    {
        return activePlugins;
    }
};
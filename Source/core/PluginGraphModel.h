#pragma once

#include <algorithm>
#include <juce_audio_processors/juce_audio_processors.h>

struct ActivePlugin
{
    const juce::PluginDescription* desc = nullptr;
    juce::String displayName = desc ? desc->name : "Unknown Plugin";
    bool bypassed = false;
    int chainIndex = -1;
    std::optional<juce::MemoryBlock> state = std::nullopt;
};

using PluginMap = std::map<juce::AudioProcessorGraph::NodeID, ActivePlugin>;

class PluginGraphModel : public juce::ChangeBroadcaster
{
private:
    PluginMap activePlugins;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginGraphModel)
public:
    PluginGraphModel() = default;

    bool addPlugin(const juce::PluginDescription& desc, 
        juce::AudioProcessorGraph::NodeID nodeId)
    {
        ActivePlugin plugin {
            .desc = &desc,
            .displayName = desc.name,
            .bypassed = false,
            .chainIndex = (int)activePlugins.size(),
            .state = std::nullopt
        };
        return addPlugin(plugin, nodeId);
    }

    bool addPlugin(const ActivePlugin& plugin, 
        juce::AudioProcessorGraph::NodeID nodeId)
    {
        if (plugin.desc == nullptr || plugin.desc->name.isEmpty())
            return false;

        if (nodeId.uid == 0)
            return false;

        activePlugins[nodeId] = plugin;

        sendChangeMessage();

        return true;
    }

    void setPluginState(juce::AudioProcessorGraph::NodeID nodeId, ActivePlugin plugin)
    {
        activePlugins[nodeId] = plugin;
        sendChangeMessage();
    }

    juce::String getDisplayName(juce::AudioProcessorGraph::NodeID nodeId) const
    {
        auto plugin = activePlugins.at(nodeId);
        return plugin.displayName;
    }

    void clearPlugins()
    {
        activePlugins.clear();
        sendChangeMessage();
    }

    const std::vector<juce::AudioProcessorGraph::NodeID> getOrderedNodeIDs() const
    {
        std::vector<juce::AudioProcessorGraph::NodeID> orderedNodeIDs;

        for (const auto& [nodeId, plugin] : activePlugins)
            orderedNodeIDs.push_back(nodeId);

        std::sort(orderedNodeIDs.begin(), orderedNodeIDs.end(), [this](const auto& a, const auto& b)
        {
            return this->activePlugins.at(a).chainIndex < this->activePlugins.at(b).chainIndex;
        });

        return orderedNodeIDs;
    }

    std::vector<ActivePlugin> getActivePlugins()
    {
        std::vector<ActivePlugin> orderedPlugins;

        for (const auto& [nodeId, plugin] : activePlugins)
            orderedPlugins.push_back(plugin);

        std::sort(orderedPlugins.begin(), orderedPlugins.end(), [](const auto& a, const auto& b)
        {
            return a.chainIndex < b.chainIndex;
        });

        return orderedPlugins;
    }

    bool removePlugin(juce::AudioProcessorGraph::NodeID nodeId)
    {
        auto result = activePlugins.erase(nodeId);
        if (result > 0)
        {
            sendChangeMessage();
            return true;
        }
        return false;
    }

    void setPluginOrder(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder)
    {
        if (!validOrder(newOrder))
            throw std::invalid_argument("Invalid plugin order: must contain all active plugins and no duplicates.");

        std::map<juce::AudioProcessorGraph::NodeID, ActivePlugin> reordered;

        int chainIndex = 0;

        for (const auto nodeId : newOrder)
        {
            // get the plugin with this nodeId
            auto& plugin = activePlugins.at(nodeId);

            plugin.chainIndex = chainIndex++;
        }

        activePlugins = std::move(reordered);

        sendChangeMessage();
    }

    bool isPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId) const
    {
        auto plugin = activePlugins.at(nodeId);

        return plugin.bypassed;
    }

    bool setPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId, bool bypassed)
    {
        auto& plugin = activePlugins.at(nodeId);
        plugin.bypassed = bypassed;
        sendChangeMessage();
        return true;
    }

    bool toggleBypass(juce::AudioProcessorGraph::NodeID nodeId)
    {
        auto& plugin = activePlugins.at(nodeId);
        plugin.bypassed = !plugin.bypassed;
        sendChangeMessage();
        return true;
    }

    bool validOrder(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder) const
    {
        // ensure that the new order contains all the active plugins and no duplicates
        if (newOrder.size() != activePlugins.size())
            return false;
        std::set<juce::AudioProcessorGraph::NodeID> uniqueIds(newOrder.begin(), newOrder.end());
        if (uniqueIds.size() != newOrder.size())
            return false;

        // ensure that all the node IDs in the new order are present in the active plugins
        for (const auto& nodeId : newOrder)
        {
            if (activePlugins.find(nodeId) == activePlugins.end())
                return false;
        }

        return true;
    }

    const juce::AudioProcessorGraph::NodeID* tryGetPluginId(juce::AudioProcessorGraph::NodeID nodeId) const
    {
        auto it = activePlugins.find(nodeId);
        if (it != activePlugins.end()) {
            return &it->first;
        }
        return nullptr;
    }

    // shows the map of the active plugins in the graph, with their node IDs as keys
    const PluginMap& getPluginMap() const
    {
        return activePlugins;
    }
};
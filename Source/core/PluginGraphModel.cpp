#include "PluginGraphModel.h"

#include <algorithm>
#include <stdexcept>

bool PluginGraphModel::addActivePlugin(const ActivePlugin& plugin,
                                 juce::AudioProcessorGraph::NodeID nodeId)
{
    if (plugin.desc.name.isEmpty())
        return false;

    if (nodeId.uid == 0)
        return false;

    ActivePlugin pluginToAdd = plugin;
    pluginToAdd.nodeId = nodeId;

    if (pluginToAdd.displayName.isEmpty())
        pluginToAdd.displayName = pluginToAdd.desc.name;

    if (pluginToAdd.chainIndex < 0)
        pluginToAdd.chainIndex = static_cast<int>(activePlugins.size());

    activePlugins.push_back(std::move(pluginToAdd));
    sendChangeMessage();
    return true;
}

bool PluginGraphModel::addPluginDescription(const juce::PluginDescription& desc,
                                 juce::AudioProcessorGraph::NodeID nodeId)
{
    ActivePlugin plugin;
    plugin.desc = desc;
    plugin.displayName = desc.name;
    plugin.bypassed = false;
    plugin.chainIndex = static_cast<int>(activePlugins.size());
    plugin.nodeId = nodeId;
    plugin.state = std::nullopt;

    return addActivePlugin(plugin, nodeId);
}

bool PluginGraphModel::isEmpty() const
{
    return activePlugins.empty();
}

void PluginGraphModel::setPluginState(juce::AudioProcessorGraph::NodeID nodeId, ActivePlugin plugin)
{
    for (auto& p : activePlugins)
    {
        if (p.nodeId == nodeId)
        {
            p.state = plugin.state;
            sendChangeMessage();
        }
    }
}

juce::String PluginGraphModel::getDisplayName(juce::AudioProcessorGraph::NodeID nodeId) const
{
    for (const auto& plugin : activePlugins)
    {
        if (plugin.nodeId == nodeId)
            return plugin.displayName;
    }

    return {};
}

void PluginGraphModel::clearPlugins()
{
    activePlugins.clear();
    sendChangeMessage();
}

bool PluginGraphModel::removePlugin(juce::AudioProcessorGraph::NodeID nodeId)
{
    auto it = std::remove_if(activePlugins.begin(),
                             activePlugins.end(),
                             [nodeId](const ActivePlugin& plugin)
                             {
                                 return plugin.nodeId == nodeId;
                             });

    if (it == activePlugins.end())
        return false;

    activePlugins.erase(it, activePlugins.end());
    sendChangeMessage();
    return true;
}

void PluginGraphModel::setPluginOrder(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder)
{
    if (!validOrder(newOrder))
        throw std::invalid_argument("Invalid plugin order: must contain all active plugins and no duplicates.");

    std::vector<ActivePlugin> newActivePlugins;
    newActivePlugins.reserve(activePlugins.size());

    int chainIndex = 0;

    for (const auto& nodeId : newOrder)
    {
        for (const auto& plugin : activePlugins)
        {
            if (plugin.nodeId == nodeId)
            {
                auto reorderedPlugin = plugin;
                reorderedPlugin.chainIndex = chainIndex++;
                newActivePlugins.push_back(std::move(reorderedPlugin));
                break;
            }
        }
    }

    activePlugins = std::move(newActivePlugins);
    sendChangeMessage();
}

bool PluginGraphModel::setPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId, bool bypassed)
{
    for (auto& plugin : activePlugins)
    {
        if (plugin.nodeId == nodeId)
        {
            plugin.bypassed = bypassed;
            sendChangeMessage();
            return true;
        }
    }

    return false;
}

bool PluginGraphModel::toggleBypass(juce::AudioProcessorGraph::NodeID nodeId)
{
    for (auto& plugin : activePlugins)
    {
        if (plugin.nodeId == nodeId)
        {
            plugin.bypassed = !plugin.bypassed;
            sendChangeMessage();
            return true;
        }
    }

    return false;
}

bool PluginGraphModel::validOrder(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder) const
{
    if (newOrder.size() != activePlugins.size())
        return false;

    std::set<juce::AudioProcessorGraph::NodeID> uniqueIds(newOrder.begin(), newOrder.end());
    if (uniqueIds.size() != newOrder.size())
        return false;

    for (const auto& nodeId : newOrder)
    {
        bool found = false;

        for (const auto& plugin : activePlugins)
        {
            if (plugin.nodeId == nodeId)
            {
                found = true;
                break;
            }
        }

        if (!found)
            return false;
    }

    return true;
}

const std::vector<ActivePlugin>& PluginGraphModel::getActivePlugins() const
{
    return activePlugins;
}

const ActivePlugin& PluginGraphModel::getActivePlugin(juce::AudioProcessorGraph::NodeID nodeId) const
{
    for (const auto& plugin : activePlugins)
    {
        if (plugin.nodeId == nodeId)
            return plugin;
    }

    throw std::runtime_error("Plugin nodeId not found in graph model");
}

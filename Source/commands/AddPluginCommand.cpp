#include "AddPluginCommand.h"

bool AddPluginCommand::perform()
{
    auto result = audioEngine.addPlugin(desc);

    if (!result.ok)
        return false;

    audioEngine.sendChangeMessage(); // Notify listeners that a new plugin has been added
    addedNodeId = result.value->nodeID;
    return true;
}

bool AddPluginCommand::undo()
{
    if (!addedNodeId)
    {
        return false;
    }
    auto status = audioEngine.removePlugin(*addedNodeId);
    if (!status.ok)
        return false;

    audioEngine.sendChangeMessage(); // Notify listeners that the plugin has been removed
    return true;
}

juce::String AddPluginCommand::getName() const
{
    return "Add Plugin: " + desc.name;
}

std::optional<juce::AudioProcessorGraph::NodeID> AddPluginCommand::getAddedNodeId() const
{
    return addedNodeId;
}

int AddPluginCommand::getSizeInUnits()
{
    return 10; // Arbitrary size for the command, can be adjusted based on actual memory usage
}


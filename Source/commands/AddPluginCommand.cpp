#include "AddPluginCommand.h"

bool AddPluginCommand::perform()
{
    if (desc.numInputChannels <= 0)
    {
        return false;
    }

    auto result = audioEngine.addPlugin(desc, formatManager);

    if (!result.ok)
        return false;

    addedNodeId = result.value->nodeID;
    return true;
}

bool AddPluginCommand::undo()
{
    if (!addedNodeId.has_value())
    {
        return false;
    }
    auto status = audioEngine.removePlugin(*addedNodeId);
    if (!status.ok)
        return false;

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


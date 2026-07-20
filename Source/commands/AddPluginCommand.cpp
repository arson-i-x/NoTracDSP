#include "AddPluginCommand.h"

bool AddPluginCommand::perform()
{
    auto result = controller.addPlugin(desc);

    if (!result.ok)
    {
        DBG("Failed to add plugin " + desc.name + ": " + result.error);
        return false;
    }

    addedNodeId = result.value;
        
    return true;
}

bool AddPluginCommand::undo()
{
    if (!addedNodeId)
    {
        return false;
    }
    auto status = controller.removePlugin(*addedNodeId);
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


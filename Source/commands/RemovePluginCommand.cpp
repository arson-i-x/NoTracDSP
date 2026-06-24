#include "RemovePluginCommand.h"

bool RemovePluginCommand::perform()
{
    auto snapshot = audioEngine.getPluginSnapshot(nodeId);
    if (!snapshot.ok)
        return false;

    removedPlugin = snapshot.value;

    auto status = audioEngine.removePlugin(nodeId);
    if (!status.ok && !removedPlugin.has_value())
    {
        DBG("Cannot remove plugin: " + status.error);
        return false;
    }
    return true;
}

bool RemovePluginCommand::undo()
{
    if (!removedPlugin.has_value())
    {
        DBG("Cannot undo RemovePluginCommand: no removed plugin snapshot.");
        return false;
    }

    audioEngine.restorePluginSnapshot(*removedPlugin, formatManager);
    return true;
}

juce::String RemovePluginCommand::getName() const
{
    return "Remove Plugin";
}

int RemovePluginCommand::getSizeInUnits()
{
    return 10; // Arbitrary size for the command, can be adjusted based on actual memory usage
}
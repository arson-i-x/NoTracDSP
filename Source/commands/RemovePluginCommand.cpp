#include "RemovePluginCommand.h"

bool RemovePluginCommand::perform()
{
    removedPlugin = controller.getPluginSnapshot(nodeId);

    if (!removedPlugin)
    {
        DBG("Failed to get plugin snapshot for plugin: " + controller.getPluginDisplayName(nodeId));
        return false;
    }

    auto status = controller.removePlugin(nodeId);
    if (!status.ok)
        return false;

    return true;
}

bool RemovePluginCommand::undo()
{
    if (!removedPlugin)
    {
        DBG("Cannot undo RemovePluginCommand: no removed plugin snapshot.");
        return false;
    }

    auto restored = controller.restorePluginSnapshot(*removedPlugin);
    if (!restored.ok)
        return false;

    nodeId = restored.value;
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
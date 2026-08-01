#include "BypassPluginCommand.h"

BypassPluginCommand::BypassPluginCommand(AppController &controller,
                                         const juce::AudioProcessorGraph::NodeID nodeId): controller(controller),
                                                                                         nodeId(nodeId)
{
    oldState = controller.getPluginGraphModel().getActivePlugin(nodeId).bypassed;
    newState = !oldState;
}

bool BypassPluginCommand::perform()
{
    return controller.setPluginBypassed(nodeId, newState).ok;
}

bool BypassPluginCommand::undo()
{
    auto status = controller.setPluginBypassed(nodeId, oldState);
    DBG ("Undo BypassPluginCommand: " + (status.ok ? "Success" : "Failure: " + status.error));
    return status.ok;
}

juce::String BypassPluginCommand::getName() const
{
    return "Set Plugin Bypass";
}

int BypassPluginCommand::getSizeInUnits()
{
    return 10; // Arbitrary size for the command, can be adjusted based on actual memory usage
}
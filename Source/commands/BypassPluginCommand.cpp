#include "BypassPluginCommand.h"

BypassPluginCommand::BypassPluginCommand(AudioEngine &engine,
                                         const juce::AudioProcessorGraph::NodeID nodeId): audioEngine(engine),
                                                                                         nodeId(nodeId)
{
    oldState = audioEngine.isPluginBypassed(nodeId);
    newState = !oldState;
}

bool BypassPluginCommand::perform()
{
    return audioEngine.setPluginBypassed(nodeId, newState).ok;
}

bool BypassPluginCommand::undo()
{
    auto status = audioEngine.setPluginBypassed(nodeId, oldState);
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
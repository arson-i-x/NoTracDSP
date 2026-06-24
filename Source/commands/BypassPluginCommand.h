#include "core/AudioEngine.h"
#include "commands/AppCommand.h"

class BypassPluginCommand : public AppCommand
{
public:
    BypassPluginCommand(AudioEngine& engine,
                     juce::AudioProcessorGraph::NodeID nodeId);
    ~BypassPluginCommand() = default;
    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override;
    juce::String getName() const override;

private:
    AudioEngine& audioEngine;
    juce::AudioProcessorGraph::NodeID nodeId;

    bool oldState = false;
    bool newState = false;
};
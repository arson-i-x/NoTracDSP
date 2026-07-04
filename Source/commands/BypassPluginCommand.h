#include "core/AudioEngine.h"
#include "commands/AppCommand.h"

class BypassPluginCommand : public AppCommand
{
private:
    AudioEngine& audioEngine;
    const juce::AudioProcessorGraph::NodeID nodeId;

    bool oldState = false;
    bool newState = false;
public:
    BypassPluginCommand(AudioEngine& engine,
                     const juce::AudioProcessorGraph::NodeID nodeId);

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override;
    juce::String getName() const override;
};
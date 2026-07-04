#pragma once

#include "core/AudioEngine.h"
#include "commands/AppCommand.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <optional>

class RemovePluginCommand : public AppCommand
{
public:
    RemovePluginCommand(AudioEngine &engine,
                                         juce::AudioProcessorGraph::NodeID nodeId)
    : audioEngine(engine),
      nodeId(nodeId)
    {
    }
    ~RemovePluginCommand() = default;
    bool perform() override;
    bool undo() override;
    juce::String getName() const override;
    int getSizeInUnits() override;

private:
    AudioEngine& audioEngine;
    juce::AudioProcessorGraph::NodeID nodeId;

    std::optional<PluginSnapshot> removedPlugin;
};
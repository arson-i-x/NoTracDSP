#pragma once

#include "core/AudioEngine.h"
#include "commands/AppCommand.h"
#include "ui/PluginWindow.h"
#include <juce_audio_processors/juce_audio_processors.h>

class AddPluginCommand : public AppCommand
{
public:
    AddPluginCommand(AudioEngine& engine,
                     juce::PluginDescription desc)
        : audioEngine(engine),
          formatManager(engine.getPluginFormatManager()),
          desc(std::move(desc))
    {
    }

    bool perform() override;

    bool undo() override;

    int getSizeInUnits() override;
    
    juce::String getName() const override;

    std::optional<juce::AudioProcessorGraph::NodeID> getAddedNodeId() const;

private:
    AudioEngine& audioEngine;
    juce::AudioPluginFormatManager& formatManager;
    juce::PluginDescription desc;

    std::optional<juce::AudioProcessorGraph::NodeID> addedNodeId;
};
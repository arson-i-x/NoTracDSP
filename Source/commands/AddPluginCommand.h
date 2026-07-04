#pragma once

#include "core/AudioEngine.h"
#include "commands/AppCommand.h"
#include "ui/PluginWindow.h"
#include <juce_audio_processors/juce_audio_processors.h>

class AddPluginCommand : public AppCommand
{
private:
    // references to the audio engine and format manager
    AudioEngine& audioEngine;

    // store a copy of the plugin description to be added
    const juce::PluginDescription desc;

    // store the node ID of the added plugin for undo purposes 
    // which could be nullptr if the plugin was not added successfully
    std::optional<juce::AudioProcessorGraph::NodeID> addedNodeId;

public:
    AddPluginCommand(AudioEngine& engine,
                     const juce::PluginDescription& desc)
        : audioEngine(engine),
          desc(desc)
    {
    }
    bool perform() override;

    bool undo() override;

    int getSizeInUnits() override;
    
    juce::String getName() const override;

    std::optional<juce::AudioProcessorGraph::NodeID> getAddedNodeId() const;
};
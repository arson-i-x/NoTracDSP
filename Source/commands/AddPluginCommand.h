#pragma once

#include "core/AppController.h"
#include "commands/AppCommand.h"
#include <juce_audio_processors/juce_audio_processors.h>

class AddPluginCommand : public AppCommand
{
private:
    // references to the audio engine and format manager
    AppController& controller;

    // store a copy of the plugin description to be added
    const juce::PluginDescription desc;

    // store the node ID of the added plugin for undo purposes 
    // which could be nullptr if the plugin was not added successfully
    std::optional<juce::AudioProcessorGraph::NodeID> addedNodeId;

public:
    AddPluginCommand(AppController& controller,
                     const juce::PluginDescription& desc)
        : controller(controller),
          desc(desc)
    {
    }
    bool perform() override;

    bool undo() override;

    int getSizeInUnits() override;
    
    juce::String getName() const override;

    std::optional<juce::AudioProcessorGraph::NodeID> getAddedNodeId() const;
};
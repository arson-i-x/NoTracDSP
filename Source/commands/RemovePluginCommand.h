#pragma once

#include "core/AppController.h"
#include "commands/AppCommand.h"
#include <juce_audio_processors/juce_audio_processors.h>

class RemovePluginCommand : public AppCommand
{
public:
    RemovePluginCommand(AppController &controller,
                        juce::AudioProcessorGraph::NodeID nodeId)
    : controller(controller),
      nodeId(nodeId)
    {
    }
    ~RemovePluginCommand() = default;
    bool perform() override;
    bool undo() override;
    juce::String getName() const override;
    int getSizeInUnits() override;

private:
    AppController& controller;
    juce::AudioProcessorGraph::NodeID nodeId;

    std::unique_ptr<ActivePlugin> removedPlugin;
};
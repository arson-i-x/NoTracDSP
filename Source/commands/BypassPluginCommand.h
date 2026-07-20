#pragma once

#include "core/AppController.h"
#include "commands/AppCommand.h"

class BypassPluginCommand : public AppCommand
{
private:
    AppController& controller;
    const juce::AudioProcessorGraph::NodeID nodeId;

    bool oldState = false;
    bool newState = false;
public:
    BypassPluginCommand(AppController& controller,
                     const juce::AudioProcessorGraph::NodeID nodeId);

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override;
    juce::String getName() const override;
};
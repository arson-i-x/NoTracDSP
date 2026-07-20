#pragma once

#include "core/AppController.h"
#include "commands/AppCommand.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <optional>

class SetPluginOrderCommand : public AppCommand
{
public:
    SetPluginOrderCommand(
        AppController& app, 
        const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder
    )
        : app(app), newOrder(newOrder)
    {
        oldOrder = app.getPluginOrder();
    }
    ~SetPluginOrderCommand() = default;
    bool perform() override;
    bool undo() override;
    juce::String getName() const override;
    int getSizeInUnits() override;

private:
    AppController& app;

    std::optional<std::vector<juce::AudioProcessorGraph::NodeID>> oldOrder;
    std::vector<juce::AudioProcessorGraph::NodeID> newOrder;
};
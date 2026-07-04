#pragma once

#include "core/AudioEngine.h"
#include "commands/AppCommand.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <optional>

class SetPluginOrderCommand : public AppCommand
{
public:
    SetPluginOrderCommand(
        AudioEngine& engine, 
        const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder
    )
        : audioEngine(engine), newOrder(newOrder)
    {
        oldOrder = audioEngine.getPluginOrder();
    }
    ~SetPluginOrderCommand() = default;
    bool perform() override;
    bool undo() override;
    juce::String getName() const override;
    int getSizeInUnits() override;

private:
    AudioEngine& audioEngine;

    std::optional<std::vector<juce::AudioProcessorGraph::NodeID>> oldOrder;
    std::vector<juce::AudioProcessorGraph::NodeID> newOrder;
};
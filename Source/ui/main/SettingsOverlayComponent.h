#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "core/MidiMapping.h"

class SettingsOverlayComponent : public juce::Component
{
public:
    SettingsOverlayComponent();

    ~SettingsOverlayComponent()
    {
        setVisible(false);
    }

    void openForPlugin(juce::AudioProcessorGraph::NodeID nodeId,
                       const juce::String& pluginName);

    std::optional<juce::AudioProcessorGraph::NodeID> getTargetNodeId() const;

    int getSelectedSwitchNumber() const;

    void paint(juce::Graphics& g) override;

    void resized() override;

    std::function<void()> onClosed;

    // Called when user clicks software switch 1-8
    std::function<void(int switchNumber)> onSwitchSelected;

private:
    void updateStatus();

    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::TextButton closeButton;

    std::vector<std::unique_ptr<juce::TextButton>> switchButtons;

    std::optional<juce::AudioProcessorGraph::NodeID> targetNodeId;
    juce::String targetPluginName;

    int selectedSwitchIndex = -1;
};
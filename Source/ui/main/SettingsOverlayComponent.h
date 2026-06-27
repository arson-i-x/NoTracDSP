#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "core/MidiMapping.h"
#include "core/AudioEngine.h"

class SettingsOverlayComponent : public juce::Component,
                                 private juce::ChangeListener
{
public:
    SettingsOverlayComponent(AudioEngine& engine);

    ~SettingsOverlayComponent()
    {
        setVisible(false);
        deviceSelector.reset();
        audioEngine.removeStatusListener(this);
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

    void changeListenerCallback(juce::ChangeBroadcaster *) override;

private:
    void updateStatus();
    void createDeviceSelectorUI();
    void createDeviceStatusUI();
    void updateDeviceLabels();

    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::TextButton closeButton;

    std::vector<std::unique_ptr<juce::TextButton>> switchButtons;

    std::optional<juce::AudioProcessorGraph::NodeID> targetNodeId;
    juce::String targetPluginName;

    int selectedSwitchIndex = -1;

    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;

    // Device status labels
    juce::Label deviceTypeLabel;
    juce::Label deviceNameLabel;
    juce::Label deviceFormatLabel;
    juce::Label deviceChannelLabel;
    juce::Label deviceLatencyLabel;
    juce::Label deviceStatusLabel;

    AudioEngine& audioEngine;
};
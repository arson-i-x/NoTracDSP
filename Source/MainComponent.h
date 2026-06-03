#pragma once

#include "AudioEngine.h"

#include <memory>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent final : public juce::Component,
                            private juce::ChangeListener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    AudioEngine audioEngine;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;

    juce::Label titleLabel;
    juce::Label gainLabel;
    juce::Slider gainSlider;
    juce::Label gainValueLabel;
    juce::Label deviceTypeLabel;
    juce::Label deviceNameLabel;
    juce::Label deviceFormatLabel;
    juce::Label deviceChannelLabel;
    juce::Label deviceLatencyLabel;
    juce::Label deviceStatusLabel;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void updateDeviceLabels();
    void updateGainReadout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
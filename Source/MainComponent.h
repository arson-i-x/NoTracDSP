#pragma once

#include "AudioEngine.h"
#include <thread>
#include <memory>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginWindow.h"
#include "PluginListWindow.h"
#include "PluginGraphViewComponent.h"
#include "MidiMapping.h"
#include "MidiMappingManager.h"
#include "SettingsOverlayComponent.h"

class MainComponent final : public juce::Component,
                            private juce::ChangeListener,
                            private juce::MidiInputCallback
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void scanForPlugins();
    void loadFirstPlugin();
    void showPluginWindow(juce::AudioProcessorGraph::NodeID nodeId);
    void removePlugin(juce::AudioProcessorGraph::NodeID nodeId);
    void refreshGraphView();
    void showGraphView();
    void hideGraphView();
    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message);
    void openFirstMidiInput();
    void startMidiLearnForPlugin(juce::AudioProcessorGraph::NodeID nodeId);
    MidiTrigger midiTriggerForSoftwareSwitch(int switchNumber);
private:
    void createPluginGraphViewWithCallbacks();
    void createDeviceSelectorUI();
    void createPluginListButton();
    void createCustomFXUI();
    void createDeviceStatusUI();
    void createMasterGainUI();

    AudioEngine audioEngine;
        
    MidiMappingManager midiMappingManager;

    std::unique_ptr<juce::MidiInput> midiInput;

    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;

    SettingsOverlayComponent settingsOverlay;

    std::optional<juce::AudioProcessorGraph::NodeID> midiLearnTarget;

    // UI components
    juce::Label titleLabel;

    juce::TextButton openPluginListWindow;
    std::unique_ptr<PluginListWindow> pluginListWindow;

    // juce::TextButton showPluginGraphViewButton;
    PluginGraphViewComponent pluginGraphViewComponent;

        // temporary first plugin window
    std::map<juce::AudioProcessorGraph::NodeID,
         std::unique_ptr<PluginWindow>> pluginWindows;

    // Delay controls
    juce::Slider delayTimeSlider;
    juce::Label delayTimeLabel;
    juce::Label delayTimeValueLabel;

    juce::Slider delayMixSlider;
    juce::Label delayMixLabel;
    juce::Label delayMixValueLabel;

    // Gain controls
    juce::Label gainLabel;
    juce::Slider gainSlider;
    juce::Label gainValueLabel;

    // Device status labels
    juce::Label deviceTypeLabel;
    juce::Label deviceNameLabel;
    juce::Label deviceFormatLabel;
    juce::Label deviceChannelLabel;
    juce::Label deviceLatencyLabel;
    juce::Label deviceStatusLabel;

    double scanProgress = 0.0;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void updateDeviceLabels();
    // void updateScannerReadouts();
    void choosePluginDirectory();
    // void chooseDeadMansPedalFile();
    void updateGainReadout();
    void updateDelayReadout();
    void getSelectedPlugin();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
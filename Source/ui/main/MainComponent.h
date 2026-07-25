#pragma once

#include "core/AppController.h"
#include <memory>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PresetManagerComponent.h"
#include "PluginGraphViewComponent.h"
#include "SettingsOverlayComponent.h"

#include "core/MidiMapping.h"
#include "core/MidiMappingManager.h"
#include "core/AppMessageBus.h"

#include "ui/pluginList/PluginListBoxComponent.h"
#include "ui/ResourcesHelper.h"

class MainComponent final : public juce::Component,
                            private juce::ChangeListener,
                            private juce::MidiInputCallback
{
public:
    MainComponent() :
        presetManagerComponent(app),
        pluginListBoxComponent(app.getKnownPluginList()),
        pluginGraphViewComponent(app)
    {
        // GUI thread: build the control surface and wire it to the audio engine.
        setOpaque (true);
        setWantsKeyboardFocus (true);
        
        createTitleLabel();
        createSettingsOverlay();
        createMasterGainUI();
        createCustomFXUI();
        // createDeviceStatusUI();
        // createDeviceSelectorUI();
        createPluginListButton();
        createPluginGraphViewWithCallbacks();
        createUndoRedoButtons();
        
        openFirstMidiInput();
        
        updateGainReadout();
        updateDelayReadout();
        
        addAndMakeVisible(presetManagerComponent);

        setSize (1440, 900);

        AppMessageBus::getInstance().onMessage = [](const AppMessage& message)
        {
            juce::String severityStr = (message.severity == AppMessageSeverity::info) ? "INFO" :
                                    (message.severity == AppMessageSeverity::warning) ? "WARNING" :
                                    (message.severity == AppMessageSeverity::error) ? "ERROR" : "UNKNOWN";
                                    
            juce::Logger::writeToLog("[" + severityStr + "]" + message.title + ": " + message.message);
            juce::AlertWindow::showMessageBoxAsync(
                message.severity == AppMessageSeverity::error ? juce::AlertWindow::AlertIconType::WarningIcon
                                                               : juce::AlertWindow::AlertIconType::InfoIcon,
                message.title,
                message.message);
        };
    }
    ~MainComponent() override 
    {
        app.shutdown();
        pluginWindows.clear();
        devices.clear();
        midiInput.reset();
        midiLearnTarget.reset();
        AppMessageBus::getInstance().onMessage = nullptr;
    };

    void paint (juce::Graphics& g) override;
    void resized() override;
    void showPluginWindow(juce::AudioProcessorGraph::NodeID nodeId);
    void refreshGraphView();
    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message) override;
    void openFirstMidiInput();
    void startMidiLearnForPlugin(juce::AudioProcessorGraph::NodeID nodeId);
    MidiTrigger midiTriggerForSoftwareSwitch(int switchNumber);
private:
    void createPluginGraphViewWithCallbacks();
    void createPluginListButton();
    void createCustomFXUI();
    void createMasterGainUI();
    void createTitleLabel();
    void createSettingsOverlay();
    void createUndoRedoButtons();

    void openPluginListWindow();
    void closePluginListWindow();

    bool sidebarOpened = false;

    AppController app;

    PresetManagerComponent presetManagerComponent;

    ImageResources resources;
        
    MidiMappingManager midiMappingManager;

    juce::Array<juce::MidiDeviceInfo> devices;

    std::unique_ptr<juce::MidiInput> midiInput;

    SettingsOverlayComponent settingsOverlay { app };

    std::optional<juce::AudioProcessorGraph::NodeID> midiLearnTarget;

    // UI components
    juce::Label titleLabel;

    juce::TextButton openPluginListWindowButton;

    PluginListBoxComponent pluginListBoxComponent;

    // juce::TextButton showPluginGraphViewButton;
    PluginGraphViewComponent pluginGraphViewComponent;

    std::map<juce::AudioProcessorGraph::NodeID,
         std::unique_ptr<PluginWindow>> pluginWindows;

    juce::ImageButton showSettingsButton;

    juce::ImageButton undoButton;
    juce::ImageButton redoButton;
    juce::ImageButton settingsButton;

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

    void changeListenerCallback (juce::ChangeBroadcaster*) override {};

    void updateDeviceLabels();
    void updateGainReadout();
    void updateDelayReadout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
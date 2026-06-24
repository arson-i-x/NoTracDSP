#pragma once

#include "core/AudioEngine.h"
#include <thread>
#include <memory>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "ui/PluginWindow.h"
#include "PluginGraphViewComponent.h"
#include "SettingsOverlayComponent.h"

#include "ui/pluginList/PluginListWindow.h"

#include "core/MidiMapping.h"
#include "core/MidiMappingManager.h"
#include "core/AppMessageBus.h"

#include "commands/BypassPluginCommand.h"
#include "commands/AddPluginCommand.h"
#include "commands/RemovePluginCommand.h"
#include "commands/SetPluginOrderCommand.h"
// #include "commands/RestorePluginSnapshotCommand.h"

class MainComponent final : public juce::Component,
                            private juce::ChangeListener,
                            private juce::MidiInputCallback
{
public:
    MainComponent()
    {
        // GUI thread: build the control surface and wire it to the audio engine.
        setOpaque (true);
        setWantsKeyboardFocus (true);
        createTitleLabel();
        createSettingsOverlay();
        createMasterGainUI();
        createCustomFXUI();
        createDeviceStatusUI();
        createDeviceSelectorUI();
        createPluginListButton();
        createPluginGraphViewWithCallbacks();
        openFirstMidiInput();
        updateDeviceLabels();
        updateGainReadout();
        updateDelayReadout();
        setSize (1200, 800);
        audioEngine.addStatusListener (this);

        messages.onMessage = [this](const AppMessage& message)
        {
            juce::Logger::writeToLog(message.title + ": " + message.message);
            juce::AlertWindow::showMessageBoxAsync(
                message.severity == AppMessageSeverity::error ? juce::AlertWindow::AlertIconType::WarningIcon
                                                               : juce::AlertWindow::AlertIconType::InfoIcon,
                message.title,
                message.message);
        };
    }
    ~MainComponent() override 
    {
        audioEngine.removeStatusListener (this);
        deviceSelector.reset();
        pluginListWindow.reset();
        pluginWindows.clear();
    };

    void paint (juce::Graphics& g) override;
    void resized() override;
    void showPluginWindow(juce::AudioProcessorGraph::NodeID nodeId);
    void refreshGraphView();
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
    void createTitleLabel();
    void createSettingsOverlay();

    void tryRemovePluginFromGraphView(juce::AudioProcessorGraph::NodeID nodeId);
    void tryChangePluginOrderFromGraphView(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder);
    void tryChangePluginBypassStateFromGraphView(juce::AudioProcessorGraph::NodeID nodeId);
    void tryMidiMapPluginFromGraphView(juce::AudioProcessorGraph::NodeID nodeId);
    void tryAddPluginFromList(const juce::PluginDescription& description);
    void tryRestorePluginSnapshotFromGraphView(juce::AudioProcessorGraph::NodeID nodeId);

    void openPluginListWindow();
    juce::UndoManager undoManager;

    AppMessageBus messages;

    AudioEngine audioEngine { messages };

    PluginScannerCoordinator pluginScanner { messages };
        
    MidiMappingManager midiMappingManager;

    juce::Array<juce::MidiDeviceInfo> devices;

    std::unique_ptr<juce::MidiInput> midiInput;

    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;

    SettingsOverlayComponent settingsOverlay;

    std::optional<juce::AudioProcessorGraph::NodeID> midiLearnTarget;

    // UI components
    juce::Label titleLabel;

    juce::TextButton openPluginListWindowButton;
    std::unique_ptr<PluginListWindow> pluginListWindow;

    // juce::TextButton showPluginGraphViewButton;
    PluginGraphViewComponent pluginGraphViewComponent;

        // temporary first plugin window
    std::map<juce::AudioProcessorGraph::NodeID,
         std::unique_ptr<PluginWindow>> pluginWindows;

    juce::ImageButton showSettingsButton;

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
    void updateGainReadout();
    void updateDelayReadout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
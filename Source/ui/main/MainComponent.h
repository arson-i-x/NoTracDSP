#pragma once

#include "core/AudioEngine.h"
#include <thread>
#include <memory>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "ui/PluginWindow.h"
#include "PresetManagerComponent.h"
#include "PluginGraphViewComponent.h"
#include "SettingsOverlayComponent.h"

#include "ui/pluginList/PluginListWindow.h"

#include "core/MidiMapping.h"
#include "core/MidiMappingManager.h"
#include "core/AppMessageBus.h"
#include "core/Result.h"
#include "core/Status.h"

#include "commands/BypassPluginCommand.h"
#include "commands/AddPluginCommand.h"
#include "commands/RemovePluginCommand.h"
#include "commands/SetPluginOrderCommand.h"
// #include "commands/RestorePluginSnapshotCommand.h"

#include "ui/ResourcesHelper.h"

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
        // createDeviceStatusUI();
        // createDeviceSelectorUI();
        createPluginListButton();
        createPluginGraphViewWithCallbacks();
        createUndoRedoButtons();
        openFirstMidiInput();
        updateGainReadout();
        updateDelayReadout();
        createPresetManagerBox();
        setSize (1440, 900);

        AppMessageBus::getInstance().onMessage = [this](const AppMessage& message)
        {
            juce::Logger::writeToLog("[USER MESSAGE]" + message.title + ": " + message.message);
            juce::AlertWindow::showMessageBoxAsync(
                message.severity == AppMessageSeverity::error ? juce::AlertWindow::AlertIconType::WarningIcon
                                                               : juce::AlertWindow::AlertIconType::InfoIcon,
                message.title,
                message.message);
        };
    }
    ~MainComponent() override 
    {
        audioEngine.shutdown();
        pluginWindows.clear();
        undoManager.clearUndoHistory();
        devices.clear();
        midiInput.reset();
        midiLearnTarget.reset();
        AppMessageBus::getInstance().onMessage = nullptr;
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
    void createPluginListButton();
    void createCustomFXUI();
    void createMasterGainUI();
    void createTitleLabel();
    void createSettingsOverlay();
    void createUndoRedoButtons();
    void createPresetManagerBox();

    void tryRemovePluginFromGraphView(juce::AudioProcessorGraph::NodeID nodeId);
    void tryChangePluginOrderFromGraphView(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder);
    void tryChangePluginBypassStateFromGraphView(juce::AudioProcessorGraph::NodeID nodeId);
    void tryMidiMapPluginFromGraphView(juce::AudioProcessorGraph::NodeID nodeId);
    void tryAddPluginFromList(const juce::PluginDescription& description);
    void tryRestorePluginSnapshotFromGraphView(juce::AudioProcessorGraph::NodeID nodeId);

    void openPluginListWindow();
    void closePluginListWindow();

    bool sidebarOpened = false;

    juce::UndoManager undoManager;

    PresetManagerComponent presetManagerComponent { audioEngine, undoManager };

    ImageResources resources;

    AudioEngine audioEngine;
        
    MidiMappingManager midiMappingManager;

    juce::Array<juce::MidiDeviceInfo> devices;

    std::unique_ptr<juce::MidiInput> midiInput;

    SettingsOverlayComponent settingsOverlay { audioEngine };

    std::optional<juce::AudioProcessorGraph::NodeID> midiLearnTarget;

    // UI components
    juce::Label titleLabel;

    juce::TextButton openPluginListWindowButton;
    // std::unique_ptr<PluginListWindow> pluginListWindow;
    PluginListBoxComponent pluginListBoxComponent { audioEngine.knownPluginList };

    // juce::TextButton showPluginGraphViewButton;
    PluginGraphViewComponent pluginGraphViewComponent;

        // temporary first plugin window
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
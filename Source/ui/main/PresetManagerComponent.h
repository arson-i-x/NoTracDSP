#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "core/AudioEngine.h"
#include "core/PresetManager.h"
#include "ui/ResourcesHelper.h"
#include "commands/ChangePresetCommand.h"

class PresetManagerComponent : public juce::Component,
                               private juce::ComboBox::Listener,
                               public juce::ChangeListener
{
private:
    juce::ImageButton savePresetButton;
    juce::ComboBox presetComboBox;

    ImageResources resources;

    AudioEngine& audioEngine;
    juce::UndoManager& undoManager;
    PresetManager presetManager;
public:
    PresetManagerComponent(AudioEngine& engine, juce::UndoManager& undoManager);
    ~PresetManagerComponent();
    void openRenameWindow();
    void resized() override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void updatePresetComboBox();
    std::function<void(juce::ValueTree)> onPresetChanged; // Callback for when presets are changed
};
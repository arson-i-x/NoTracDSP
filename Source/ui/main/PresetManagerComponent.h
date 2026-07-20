#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "core/PresetManager.h"
#include "core/AppController.h"
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

    AppController& app;
public:
    PresetManagerComponent(AppController& appController);
    ~PresetManagerComponent();
    void openRenameWindow();
    void resized() override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void updatePresetComboBox();
};
#include "PresetManagerComponent.h"

void PresetManagerComponent::resized()
{
    auto area = getLocalBounds().reduced(20);
    auto presetArea = area.removeFromLeft(getWidth() - 120);
    auto buttonArea = area.removeFromRight(100);
    presetComboBox.setBounds(presetArea);
    savePresetButton.setBounds(buttonArea);
}

PresetManagerComponent::PresetManagerComponent(AppController& appController)
    : app(appController),
      savePresetButton("Save Preset"),
      presetComboBox("UserPresets")
{
    app.setPresetManagerCallback([this]() { updatePresetComboBox(); });

    presetComboBox.setEditableText(false);
    presetComboBox.addListener(this);

    // presetManager.addChangeListener(this); // Listen for changes in the preset manager

    auto saveImage = resources.getIcon(IconType::SavePreset);

    savePresetButton.setImages(
        false, true, true,
        saveImage, 1.0f, juce::Colours::transparentBlack,
        saveImage, 0.8f, juce::Colours::transparentBlack,
        saveImage, 0.5f, juce::Colours::transparentBlack);

    savePresetButton.onClick = [this]()
    {
        openRenameWindow();
    };

    savePresetButton.setTooltip("Save Preset");
    savePresetButton.setSize(getHeight(), getHeight());

    presetComboBox.setColour(
        juce::ComboBox::backgroundColourId, 
        juce::Colours::transparentBlack); 

    addAndMakeVisible(presetComboBox);
    addAndMakeVisible(savePresetButton);

    updatePresetComboBox();

    setSize(800, 80);
}

PresetManagerComponent::~PresetManagerComponent()
{
    app.removeChangeListener(this);           // Remove listener when PresetManagerComponent is destroyed to prevent dangling pointers
    
    app.setPresetManagerCallback(nullptr); // Remove the callback to prevent dangling pointers
    
    presetComboBox.removeListener(this);      // Remove listener when PresetManagerComponent is destroyed to prevent dangling pointers
}

void PresetManagerComponent::updatePresetComboBox()
{
    presetComboBox.clear(juce::dontSendNotification);

    auto presets = app.getPresets();

    for (int id = 1; id <= presets.size(); id++)
    {
        // Preset name matched by the index in the PresetList, which is 1-based
        presetComboBox.addItem(presets[id], id);
    }

    presetComboBox.setText(
        app.getCurrentPresetName(), 
        juce::dontSendNotification); // Select the current preset by default

    // reset colour to default after updating the combo box
    presetComboBox.setColour(
        juce::ComboBox::backgroundColourId, 
        juce::Colours::transparentBlack); 
}

void PresetManagerComponent::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (source == &app)
    {
        // Update the combo box to show the current preset is modified
        presetComboBox.setText(
            app.getCurrentPresetName() + "*", 
            juce::dontSendNotification); 
    }
}

void PresetManagerComponent::openRenameWindow()
{
    JUCE_ASSERT_MESSAGE_THREAD; // Ensure this is called from the main thread

    juce::AlertWindow renameWindow("Save Preset", "Enter a name for the preset:", juce::AlertWindow::NoIcon);
    renameWindow.addTextEditor("presetName", presetComboBox.getText(), "Preset Name:");
    renameWindow.addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    renameWindow.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    if (renameWindow.runModalLoop() == 1) // If "Save" button is clicked
    {
        auto presetName = renameWindow.getTextEditor("presetName")->getText();
        if (!presetName.isEmpty())
        {
            auto state = app.createPresetState();
            auto result = app.savePreset(presetName, state);
            if (result.ok)
            {                             
                presetComboBox.setText(presetName, juce::dontSendNotification); // Update the combo box to show the new preset
            }
            else
            {
                AppMessageBus::getInstance().error("Error Saving Preset", result.error);
            }
        }
    }
}

void PresetManagerComponent::comboBoxChanged(juce::ComboBox *comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &presetComboBox)
    {
        int selectedIndex = presetComboBox.getSelectedId();

        if (selectedIndex < 1 || selectedIndex > app.getPresets().size())
            return;

        if (selectedIndex >= 1 && selectedIndex <= app.getPresets().size())
            app.setCurrentPreset(selectedIndex);
    }
}
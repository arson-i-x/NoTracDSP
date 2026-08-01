#include "PluginListBoxComponent.h"

PluginListBoxComponent::PluginListBoxComponent(juce::KnownPluginList& list)
        : knownPluginList(list),
            model(knownPluginList),
            pluginScanner(knownPluginList)
{
    pluginListBox.setMultipleSelectionEnabled(false);
    pluginListBox.setClickingTogglesRowSelection(false);
        pluginListBox.setModel(&model);
    setVisible(true);

    pluginScannerLabel.setText ("Plugin Scanner", juce::dontSendNotification);
    pluginScannerLabel.setJustificationType (juce::Justification::centredLeft);
    pluginScannerLabel.setFont (juce::Font { juce::FontOptions (20.0f, juce::Font::bold) });
    pluginScannerLabel.setColour (juce::Label::textColourId, juce::Colour (0xffdbe4ee));
    addAndMakeVisible (pluginScannerLabel);

    pluginDirectoryLabel.setText ("Scan Directory", juce::dontSendNotification);
    pluginDirectoryLabel.setJustificationType (juce::Justification::centredLeft);
    pluginDirectoryLabel.setColour (juce::Label::textColourId, juce::Colour (0xffc7d0db));
    addAndMakeVisible (pluginDirectoryLabel);

    pluginDirectoryValueLabel.setJustificationType (juce::Justification::centredLeft);
    pluginDirectoryValueLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8bd3ff));
    addAndMakeVisible (pluginDirectoryValueLabel);

    choosePluginDirectoryButton.setButtonText ("Choose Folder...");
    choosePluginDirectoryButton.onClick = [this] 
    { 
        directoryManager.onDirectoryChosen = [this](const juce::File& selectedDir) 
        {
            selectedPluginDirectory = selectedDir;
            pluginDirectoryValueLabel.setText(selectedPluginDirectory.getFullPathName(), juce::dontSendNotification);
            updateScanSettings();
        };
        directoryManager.chooseDirectory();
    };
    addAndMakeVisible (choosePluginDirectoryButton);

    searchRecursivelyToggle.setButtonText ("Search recursively");
    searchRecursivelyToggle.setToggleState (true, juce::dontSendNotification);
    searchRecursivelyToggle.onClick = [this] { updateScanSettings(); };
    addAndMakeVisible (searchRecursivelyToggle);

    rescanToggle.setButtonText ("Rescan already listed plugins");
    rescanToggle.setToggleState (false, juce::dontSendNotification);
    rescanToggle.onClick = [this] { updateScanSettings(); };
    addAndMakeVisible (rescanToggle);

    allowAsyncToggle.setButtonText ("Allow async plugin instantiation");
    allowAsyncToggle.setToggleState (true, juce::dontSendNotification);
    allowAsyncToggle.onClick = [this] { updateScanSettings(); };
    addAndMakeVisible (allowAsyncToggle);

    scanPluginsButton.setButtonText ("Scan Plugins");
    scanPluginsButton.onClick = [this] { scanForPlugins(); };
    addAndMakeVisible (scanPluginsButton);

    loadPluginButton.setButtonText("Load Selected");
    loadPluginButton.onClick = [this] { choosePlugin(); };
    addAndMakeVisible(loadPluginButton);

    addAndMakeVisible (pluginListBox);

    updateScanSettings();

    refreshPluginList();
}

void PluginListBoxComponent::updateScanSettings() 
{
    settings.directory = selectedPluginDirectory;
    settings.recursive = searchRecursivelyToggle.getToggleState();
    settings.dontRescanIfAlreadyInList = !rescanToggle.getToggleState();
    settings.allowAsyncInstantiation = allowAsyncToggle.getToggleState();
}

void PluginListBoxComponent::refreshPluginList()
{
    model.update();
    DBG("Updating plugin list with " + juce::String(knownPluginList.getTypes().size()) + " plugins.");
    pluginListBox.updateContent();
    repaint();
}

void PluginListBoxComponent::scanForPlugins() {
    DBG("Scanning for plugins in directory: " + selectedPluginDirectory.getFullPathName());

    scanPluginsButton.setEnabled (false);
    scanPluginsButton.setButtonText ("Scanning...");

    pluginScanner.startScan(settings, 
        [this](const juce::String& text, bool enabled)
        {
            scanPluginsButton.setButtonText(text);
            scanPluginsButton.setEnabled(enabled);
        },
        [this]()
        {
            refreshPluginList();
            AppMessageBus::getInstance().info(
                "Plugin scan completed", 
                "Found " + juce::String(knownPluginList.getTypes().size()) + " plugins."
            );
        });

    resized(); // Trigger a layout update to show the progress bar immediately
}

void PluginListBoxComponent::resized() {
    auto area = getLocalBounds().reduced (28);
    area.removeFromTop (48);
    auto rightSide = area.removeFromRight (32);
    auto topRightCorner = rightSide.removeFromTop (32);
    closeButton.setBounds(topRightCorner);
    
    auto scannerArea = area.removeFromTop (250);
    pluginScannerLabel.setBounds (scannerArea.removeFromTop (30));

    auto scannerContent = scannerArea.reduced (0, 4);
    auto scannerLeft = scannerContent.removeFromLeft (420);
    scannerContent.removeFromLeft (12);

    auto directoryRow = scannerLeft.removeFromTop (30);
    pluginDirectoryLabel.setBounds (directoryRow.removeFromLeft (140));
    choosePluginDirectoryButton.setBounds (directoryRow.removeFromRight (140));
    pluginDirectoryValueLabel.setBounds (directoryRow);

    scannerLeft.removeFromTop (12);

    searchRecursivelyToggle.setBounds (scannerLeft.removeFromTop (24));
    scannerLeft.removeFromTop (6);
    rescanToggle.setBounds (scannerLeft.removeFromTop (24));
    scannerLeft.removeFromTop (6);
    allowAsyncToggle.setBounds (scannerLeft.removeFromTop (24));
    scannerLeft.removeFromTop (12);
    scanPluginsButton.setBounds (scannerLeft.removeFromTop (32).removeFromLeft (160));

    area.removeFromTop (12);

    auto scannedPluginsArea = area.removeFromTop (250);
    pluginListBox.setBounds (scannedPluginsArea);

    auto loadArea = area.removeFromTop(32);
    loadPluginButton.setBounds(loadArea.removeFromLeft(160));
}

void PluginListBoxComponent::choosePlugin() 
{
    if (!onPluginChosen)
        return;

    const int selectedRow = pluginListBox.getSelectedRow();
    if (selectedRow < 0)
        return;

    onPluginChosen(model.getPluginDescription(selectedRow));
    }
#include "PluginListBoxComponent.h"

PluginListBoxComponent::PluginListBoxComponent(juce::KnownPluginList& knownPluginList) 
            : knownPluginList(knownPluginList)
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
    auto titleArea = area.removeFromTop (48);
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

    pluginListBox.setBounds(scannedPluginsArea);

    auto loadArea = area.removeFromTop(32);
    loadPluginButton.setBounds(loadArea.removeFromLeft(160));
}

void PluginListBoxComponent::choosePlugin() 
{
    int selectedRow = pluginListBox.getSelectedRow();

    if (selectedRow < 0)
        return;

    const auto& types = model.getPluginDescriptions();

    if (selectedRow >= types.size())
        return;

    if (onPluginChosen)
        onPluginChosen(types[selectedRow]);
}

juce::Array<juce::PluginDescription> PluginListModel::getPluginDescriptions() const
    {
        return pluginList.getTypes();
    }

    
int PluginListModel::getNumRows()
   {
        return pluginList.getTypes().size();
    }

void PluginListModel::paintListBoxItem(int rowNumber,
                                        juce::Graphics& g,
                                        int width,
                                        int height,
                                        bool rowIsSelected)

    {

        auto types = pluginList.getTypes();

        if (rowNumber >= types.size())
            return;

        if (rowIsSelected)
            g.fillAll(juce::Colours::darkgrey);

        g.setColour(juce::Colours::white);
        g.drawText(types[rowNumber].name, 4, 0, width, height,
                    juce::Justification::centredLeft);

        g.drawText(types[rowNumber].name,
                   4, 0,
                   width, height,
                   juce::Justification::centredLeft);
    }
    // {
    //     if (pluginList == nullptr || rowNumber < 0 || rowNumber >= pluginList->getTypes().size())
    //         return;

    //     const auto& desc = pluginList->getTypes()[rowNumber];

    //     if (rowIsSelected)
    //         g.fillAll(juce::Colour(0xff2b3440));

    //     g.setColour(juce::Colour(0xff8bd3ff));
    //     g.setFont(juce::Font { juce::FontOptions (16.0f, juce::Font::bold) });
    //     g.drawText(desc.name, 4, 0, width - 4, height / 2, juce::Justification::centredLeft);

    //     g.setColour(juce::Colour(0xffc7d0db));
    //     g.setFont(juce::Font { juce::FontOptions (14.0f) });
    //     g.drawText(desc.pluginFormatName + " - " + desc.fileOrIdentifier, 4, height / 2, width - 4, height / 2, juce::Justification::centredLeft);
    // }
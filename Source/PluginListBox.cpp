#include "PluginListBox.h"

PluginListBox::PluginListBox()
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
    choosePluginDirectoryButton.onClick = [this] { choosePluginDirectory(); };
    addAndMakeVisible (choosePluginDirectoryButton);

    searchRecursivelyToggle.setButtonText ("Search recursively");
    searchRecursivelyToggle.setToggleState (true, juce::dontSendNotification);
    addAndMakeVisible (searchRecursivelyToggle);

    rescanToggle.setButtonText ("Rescan already listed plugins");
    rescanToggle.setToggleState (false, juce::dontSendNotification);
    addAndMakeVisible (rescanToggle);

    allowAsyncToggle.setButtonText ("Allow async plugin instantiation");
    allowAsyncToggle.setToggleState (true, juce::dontSendNotification);
    addAndMakeVisible (allowAsyncToggle);

    scanPluginsButton.setButtonText ("Scan Plugins");
    scanPluginsButton.onClick = [this] { scanForPlugins(); };
    addAndMakeVisible (scanPluginsButton);

    loadPluginButton.setButtonText("Load Selected");
    loadPluginButton.onClick = [this] { loadPlugin(); };
    addAndMakeVisible(loadPluginButton);

    addAndMakeVisible (pluginListBox);

    scanForPlugins(); // perform an initial scan to populate the list with any already known plugins
}

PluginListBox::~PluginListBox()
{
    setVisible(false);
    fileChooser.reset();
}

void PluginListBox::setPluginList(const juce::KnownPluginList& newList)
{
    DBG("Updating plugin list with " + juce::String(newList.getTypes().size()) + " plugins.");
    model.setPluginList(newList);
    pluginScanner.saveKnownPluginList();
    pluginListBox.updateContent();
    repaint();
}

void PluginListBox::scanForPlugins() {
    DBG("Scanning for plugins in directory: " + selectedPluginDirectory.getFullPathName());

    scanPluginsButton.setEnabled (false);
    scanPluginsButton.setButtonText ("Scanning...");

    PluginScannerCoordinator::ScanSettings settings;
    settings.directory = selectedPluginDirectory;
    settings.recursive = searchRecursivelyToggle.getToggleState();
    settings.dontRescanIfAlreadyInList = ! rescanToggle.getToggleState();
    settings.allowAsyncInstantiation = allowAsyncToggle.getToggleState();

    pluginScanner.startScan(settings, [this](const juce::String& text, bool enabled)
    {
            scanPluginsButton.setButtonText(text);
            scanPluginsButton.setEnabled(enabled);
            if (enabled)
            {
                setPluginList(pluginScanner.knownPluginList);     
            }
    });

    resized(); // Trigger a layout update to show the progress bar immediately
}

void PluginListBox::resized() {
    auto area = getLocalBounds().reduced (28);
    auto titleArea = area.removeFromTop (48);

    
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

void PluginListBox::choosePluginDirectory()
{
    // 1. Set up the file chooser criteria
    fileChooser = std::make_unique<juce::FileChooser> (
        "Select a Directory to Scan for Plugins",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory)
    );

    // 2. Define standard browser behavior flags
    auto chooserFlags = juce::FileBrowserComponent::openMode 
                      | juce::FileBrowserComponent::canSelectDirectories;

    // 3. Launch asynchronously without blocking the message loop
    fileChooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& chooser)
    {
        // This block executes ONLY when the user closes the window
        auto resultFile = chooser.getResult();

        selectedPluginDirectory = resultFile;
        // updateScannerReadouts();
    });
}

juce::PluginDescription PluginListBox::getSelectedPlugin() 
{
    int row = pluginListBox.getSelectedRow();

    if (row >= 0)
    {
        const auto desc = pluginScanner.knownPluginList.getTypes()[row];
        DBG("Selected plugin: " + desc.name + " at " + desc.fileOrIdentifier);
        return desc;
    }

    return juce::PluginDescription();
}

void PluginListBox::loadPlugin() 
{
    DBG("Load plugin button clicked.");
    auto desc = getSelectedPlugin();

    if (onPluginChosen)
        onPluginChosen(desc);
}
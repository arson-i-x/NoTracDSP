// Custom Plugin List box that derives from knownPluginList 
// and creates a UI box to load plugins in the engine
#pragma once

#include "scanner/PluginScannerCoordinator.h"
#include "core/DirectoryManager.h"
#include "PluginListModel.h"

class PluginListBoxComponent : public juce::Component
{
public:
    explicit PluginListBoxComponent(juce::KnownPluginList& knownPluginList);
    void resized() override;

    std::function<void(const juce::PluginDescription&)> onPluginChosen;

private:
    void choosePlugin();
    void scanForPlugins();
    void updateScanSettings();
    void refreshPluginList();

    juce::KnownPluginList& knownPluginList;
    PluginListModel model;
    juce::ListBox pluginListBox { "Plugin List", nullptr };

    DirectoryManager directoryManager;

    // Plugin scanning UI
    juce::Label pluginScannerLabel;
    juce::Label pluginDirectoryLabel;
    juce::Label pluginDirectoryValueLabel;
    juce::TextButton choosePluginDirectoryButton;

    juce::ToggleButton searchRecursivelyToggle;
    juce::ToggleButton rescanToggle;
    juce::ToggleButton allowAsyncToggle;

    juce::TextButton scanPluginsButton;

    juce::ImageButton closeButton;

    PluginScannerCoordinator pluginScanner;

    // State variables for plugin scanning
    juce::File selectedPluginDirectory;

    juce::TextButton loadPluginButton ;

    PluginScannerCoordinator::ScanSettings settings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListBoxComponent)

};
// Custom Plugin List box that derives from knownPluginList 
// and creates a UI box to load plugins in the engine
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "scanner/PluginScannerCoordinator.h"
#include "core/DirectoryManager.h"

class PluginListModel : public juce::ListBoxModel
{
public:
    PluginListModel() = default;

    ~PluginListModel() = default;

    int getNumRows() override;

    void paintListBoxItem(int rowNumber,
                          juce::Graphics& g,
                          int width,
                          int height,
                          bool rowIsSelected) override;

    void setPluginList(const juce::KnownPluginList* newList)
    {
        pluginList = newList;
    }

private:
    const juce::KnownPluginList* pluginList = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListModel)
};

class PluginListBoxComponent : public juce::Component
{
public:
    PluginListBoxComponent(AppMessageBus& msg);
    ~PluginListBoxComponent();
    void setPluginList(const juce::KnownPluginList& newList);
    void scanForPlugins();
    void resized() override;
    void loadPlugin();

    std::function<void(const juce::PluginDescription&)> onPluginChosen;
    
private:
    AppMessageBus messages;

    juce::PluginDescription getSelectedPlugin();

    PluginListModel model;
    juce::ListBox pluginListBox;

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

    PluginScannerCoordinator pluginScanner { messages };

    // State variables for plugin scanning
    juce::File selectedPluginDirectory;

    juce::TextButton loadPluginButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListBoxComponent)

};
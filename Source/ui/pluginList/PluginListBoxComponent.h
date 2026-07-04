// Custom Plugin List box that derives from knownPluginList 
// and creates a UI box to load plugins in the engine
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "scanner/PluginScannerCoordinator.h"
#include "core/DirectoryManager.h"
#include "core/Result.h"
#include "core/Status.h"
#include "commands/AppCommand.h"

class PluginListModel : public juce::ListBoxModel
{
public:
    PluginListModel(juce::KnownPluginList& pluginList) : 
        pluginList(pluginList)
    {
    };
    int getNumRows() override;

    void paintListBoxItem(int rowNumber,
                          juce::Graphics& g,
                          int width,
                          int height,
                          bool rowIsSelected) override;

    juce::Array<juce::PluginDescription> getPluginDescriptions() const;

    juce::KnownPluginList& getPluginList() const { return pluginList; }

private:
    juce::KnownPluginList& pluginList;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListModel)
};

class PluginListBoxComponent : public juce::Component
{
public:
    PluginListBoxComponent(juce::KnownPluginList& knownPluginList);

    void resized() override;

    juce::KnownPluginList& getKnownPluginList() const { return knownPluginList; }

    std::function<void(const juce::PluginDescription&)> onPluginChosen;

private:
    void choosePlugin();
    void scanForPlugins();
    void updateScanSettings();
    void refreshPluginList();
    std::optional<juce::PluginDescription> getSelectedPlugin();

    juce::KnownPluginList& knownPluginList;

    PluginListModel model { knownPluginList };
    juce::ListBox pluginListBox { "Plugin List", &model };

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

    PluginScannerCoordinator pluginScanner { knownPluginList };

    // State variables for plugin scanning
    juce::File selectedPluginDirectory;

    juce::TextButton loadPluginButton ;

    PluginScannerCoordinator::ScanSettings settings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListBoxComponent)

};
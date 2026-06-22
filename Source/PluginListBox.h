// Custom Plugin List box that derives from knownPluginList 
// and creates a UI box to load plugins in the engine
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginScannerCoordinator.h"

class PluginListModel : public juce::ListBoxModel
{
public:
    PluginListModel() = default;

    ~PluginListModel() = default;

    void setPluginList(const juce::KnownPluginList& list)
    {
        pluginList = &list;
    }

    int getNumRows() override
    {
        if (pluginList == nullptr)
            return 0;

        return pluginList->getTypes().size();
    }

    void paintListBoxItem(int rowNumber,
                          juce::Graphics& g,
                          int width,
                          int height,
                          bool rowIsSelected) override
    {
        if (pluginList == nullptr)
            return;

        auto types = pluginList->getTypes();

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

private:
    const juce::KnownPluginList* pluginList = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListModel)
};

class PluginListBox : public juce::Component
{
public:
    PluginListBox();
    ~PluginListBox();
    void setPluginList(const juce::KnownPluginList& newList);
    void choosePluginDirectory();
    void scanForPlugins();
    void resized() override;
    void loadPlugin();

    juce::AudioPluginFormatManager& getPluginFormatManager() { return pluginScanner.formatManager; };

    std::function<void(const juce::PluginDescription&)> onPluginChosen;
    
private:
    juce::PluginDescription getSelectedPlugin();

    PluginListModel model;
    juce::ListBox pluginListBox;

    // Plugin scanning UI
    juce::Label pluginScannerLabel;
    juce::Label pluginDirectoryLabel;
    juce::Label pluginDirectoryValueLabel;
    juce::TextButton choosePluginDirectoryButton;

    juce::ToggleButton searchRecursivelyToggle;
    juce::ToggleButton rescanToggle;
    juce::ToggleButton allowAsyncToggle;

    juce::TextButton scanPluginsButton;

    std::unique_ptr<juce::FileChooser> fileChooser;

    PluginScannerCoordinator pluginScanner;

    // State variables for plugin scanning
    juce::File selectedPluginDirectory;

    juce::TextButton loadPluginButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListBox)

};
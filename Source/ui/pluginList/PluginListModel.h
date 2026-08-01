#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class PluginListModel : public juce::ListBoxModel
{
public:
    explicit PluginListModel(const juce::KnownPluginList& knownPluginList);

    int getNumRows() override;

    void update();

    void paintListBoxItem(int rowNumber,
                          juce::Graphics& g,
                          int width,
                          int height,
                          bool rowIsSelected) override;

    juce::PluginDescription getPluginDescription(int rowNumber) const
    {
        return pluginDescriptions[rowNumber];
    }

private:
    juce::Array<juce::PluginDescription> pluginDescriptions;

    const juce::KnownPluginList& knownPluginList;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListModel)
};
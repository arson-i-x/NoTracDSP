#include "PluginListModel.h"

PluginListModel::PluginListModel(const juce::KnownPluginList& knownPluginList)
    : knownPluginList(knownPluginList)
{
    update();
}

int PluginListModel::getNumRows()
{
    return pluginDescriptions.size();
}

void PluginListModel::update()
{
    pluginDescriptions = knownPluginList.getTypes();
}

void PluginListModel::paintListBoxItem(int rowNumber,
                                       juce::Graphics& g,
                                       int width,
                                       int height,
                                       bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= pluginDescriptions.size())
        return;

    if (rowIsSelected)
        g.fillAll(juce::Colours::darkgrey);

    const auto& description = pluginDescriptions[rowNumber];

    if (description.fileOrIdentifier.startsWith("notrac.internal"))
    {
        g.setColour(juce::Colours::lightblue);
        g.drawText(description.name, 4, 0, width, height, juce::Justification::centredLeft);
    }
    else
    {
        g.setColour(juce::Colours::white);
        g.drawText(description.name, 4, 0, width, height, juce::Justification::centredLeft);
    }
}

#include "PluginListWindow.h"
#include "core/AudioEngine.h"

void PluginListWindow::closeButtonPressed()
{
    setVisible(false);
}

PluginListWindow::~PluginListWindow()
{
    setVisible(false);
}

PluginListWindow::PluginListWindow(juce::KnownPluginList& knownPluginList)
    : DocumentWindow("Plugin List",
                     juce::Colours::black,
                     DocumentWindow::closeButton)
{
    setContentOwned(new PluginListBoxComponent(knownPluginList), true);
    centreWithSize(700, 1000);
    setVisible(true);
}

PluginListBoxComponent* PluginListWindow::getPluginListBox() const
{
    return dynamic_cast<PluginListBoxComponent*>(getContentComponent());
}

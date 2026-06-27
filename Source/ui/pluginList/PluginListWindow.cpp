#include "PluginListWindow.h"

void PluginListWindow::closeButtonPressed()
{
    setVisible(false);
}

PluginListWindow::~PluginListWindow()
{
    setVisible(false);
}

PluginListWindow::PluginListWindow()
    : DocumentWindow("Plugin List",
                     juce::Colours::black,
                     DocumentWindow::closeButton)
{
    setContentOwned(new PluginListBoxComponent(), true);
    centreWithSize(700, 1000);
    setVisible(true);
}

PluginListBoxComponent* PluginListWindow::getPluginListBox() const
{
    return dynamic_cast<PluginListBoxComponent*>(getContentComponent());
}

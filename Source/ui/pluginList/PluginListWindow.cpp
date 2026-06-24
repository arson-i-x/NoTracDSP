#include "PluginListWindow.h"

void PluginListWindow::closeButtonPressed()
{
    setVisible(false);
}

PluginListWindow::~PluginListWindow()
{
    setVisible(false);
}

PluginListWindow::PluginListWindow(AppMessageBus& msg)
    : DocumentWindow("Plugin List",
                     juce::Colours::black,
                     DocumentWindow::closeButton),
      messages(msg)
{
    setContentOwned(new PluginListBox(messages), true);
    centreWithSize(700, 1000);
    setVisible(true);
}

PluginListBox* PluginListWindow::getPluginListBox() const
{
    return dynamic_cast<PluginListBox*>(getContentComponent());
}

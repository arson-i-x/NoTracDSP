#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginListBox.h"

class PluginListWindow : public juce::DocumentWindow
{
public:
    PluginListWindow()
        : DocumentWindow("Plugin List",
                         juce::Colours::black,
                         DocumentWindow::closeButton)
    {
        setContentOwned(new PluginListBox(), true);
        centreWithSize(700, 1000);
        setVisible(true);
    }

    PluginListBox* getPluginListBox() const
    {
        return dynamic_cast<PluginListBox*>(getContentComponent());
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }
};
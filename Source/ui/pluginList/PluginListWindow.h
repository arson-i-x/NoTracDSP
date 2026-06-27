#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginListBoxComponent.h"

class PluginListWindow : public juce::DocumentWindow
{
public:
    PluginListWindow();
    ~PluginListWindow();
    PluginListBoxComponent* getPluginListBox() const;
    void closeButtonPressed() override;
};
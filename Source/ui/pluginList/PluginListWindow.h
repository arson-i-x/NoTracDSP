#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginListBoxComponent.h"

class PluginListWindow : public juce::DocumentWindow
{
public:
    PluginListWindow(AppMessageBus& msg);
    ~PluginListWindow();
    PluginListBoxComponent* getPluginListBox() const;
    void closeButtonPressed() override;
private:
    AppMessageBus messages;
};
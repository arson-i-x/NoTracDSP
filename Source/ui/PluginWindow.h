#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
class PluginWindow : public juce::DocumentWindow
{
public:
    PluginWindow(juce::AudioProcessor* processor);
    void closeButtonPressed() override; 
    ~PluginWindow()
    {
        setVisible(false);
    }
};
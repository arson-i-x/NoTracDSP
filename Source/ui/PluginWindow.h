#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "core/AppMessageBus.h"

class PluginWindow : public juce::DocumentWindow
{
private:
    juce::AudioProcessor& processor;
public:
    PluginWindow(juce::AudioProcessor& processor);
    void closeButtonPressed() override; 
    ~PluginWindow()
    {
        setVisible(false);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginWindow)
};
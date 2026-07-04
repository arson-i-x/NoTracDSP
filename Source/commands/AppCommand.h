#pragma once

#include <juce_core/juce_core.h>

class AppCommand : public juce::UndoableAction
{
public:
    virtual juce::String getName() const = 0;
};
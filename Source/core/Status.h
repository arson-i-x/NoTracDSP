#pragma once
#include <juce_core/juce_core.h>

struct Status
{
    bool ok = true;
    juce::String error;

    static Status success()
    {
        return {true, {}};
    }

    static Status failure(juce::String e)
    {
        return {false, std::move(e)};
    }
};
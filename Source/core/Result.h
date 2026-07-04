#pragma once
#include <juce_core/juce_core.h>

template <typename T>
struct Result
{
    bool ok = false;
    T value{};
    juce::String error;

    static Result success(T v)
    {
        return {true, std::move(v), {}};
    }

    static Result failure(juce::String e)
    {
        return {false, {}, std::move(e)};
    }
};
#pragma once

#include <juce_core/juce_core.h>

class AppCommand : public juce::UndoableAction
{
public:
    template <typename T>
    struct Result
    {
        bool ok = false;
        T value {};
        juce::String error;

        static Result success(T v)
        {
            return { true, std::move(v), {} };
        }

        static Result failure(juce::String e)
        {
            return { false, {}, std::move(e) };
        }
    };

    struct Status
    {
        bool ok = true;
        juce::String error;

        static Status success()
        {
            return { true, {} };
        }

        static Status failure(juce::String e)
        {
            return { false, std::move(e) };
        }
    };
    AppCommand() = default;
    ~AppCommand() = default;
    virtual juce::String getName() const = 0;
};
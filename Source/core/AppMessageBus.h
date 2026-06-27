#pragma once

#include <juce_core/juce_core.h>

enum class AppMessageSeverity
{
    info,
    warning,
    error
};

struct AppMessage
{
    AppMessageSeverity severity;
    juce::String title;
    juce::String message;
};

class AppMessageBus
{
public:
    static AppMessageBus& getInstance()
    {
        static AppMessageBus instance;
        return instance;
    }

    std::function<void(const AppMessage&)> onMessage;

    void post(AppMessage message)
    {
        if (onMessage)
            onMessage(message);
    }

    void info(juce::String title, juce::String message)
    {
        post({ AppMessageSeverity::info, title, message });
    }

    void warning(juce::String title, juce::String message)
    {
        post({ AppMessageSeverity::warning, title, message });
    }

    void error(juce::String title, juce::String message)
    {
        post({ AppMessageSeverity::error, title, message });
    }
private:
    AppMessageBus() = default;
    ~AppMessageBus() = default;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppMessageBus)
};
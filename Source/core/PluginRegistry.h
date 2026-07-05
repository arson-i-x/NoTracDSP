#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "core/Result.h"

class PluginRegistry
{
private:
    juce::KnownPluginList knownPluginList;
    juce::AudioPluginFormatManager pluginFormatManager;

public:
    Result<std::unique_ptr<juce::AudioProcessor>> createPluginInstance(
        const juce::PluginDescription& desc,
        double sampleRate,
        int blockSize)
    {
        juce::String error;

        auto plugin = pluginFormatManager.createPluginInstance(
            desc,
            sampleRate,
            blockSize,
            error);

        if (!plugin)
        {
            DBG("Failed to load plugin: " + error);
            return Result<std::unique_ptr<juce::AudioProcessor>>::failure("Failed to load plugin: " + error);
        }

        return Result<std::unique_ptr<juce::AudioProcessor>>::success(std::move(plugin));
    }

    Result<std::unique_ptr<juce::AudioProcessor>> getPluginInstance(
        const juce::PluginDescription& desc,
        double sampleRate,
        int blockSize)
    {
        return createPluginInstance(desc, sampleRate, blockSize);
    }


    juce::KnownPluginList& getKnownPluginList() noexcept { return knownPluginList; }
    const juce::KnownPluginList& getKnownPluginList() const noexcept { return knownPluginList; }

    juce::AudioPluginFormatManager& getPluginFormatManager() noexcept { return pluginFormatManager; }
    const juce::AudioPluginFormatManager& getPluginFormatManager() const noexcept { return pluginFormatManager; }
};
    
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "core/Result.h"
#include "customProcessors/PolyphonicOctaver.h"

class PluginRegistry
{
private:
    juce::KnownPluginList knownPluginList;
    juce::AudioPluginFormatManager pluginFormatManager;

public:
    PluginRegistry()
    {
        juce::addDefaultFormatsToManager(pluginFormatManager);
    
        // add default plugins to the known plugin list
        knownPluginList.addType(PolyphonicOctaver::getPluginDescription());
    }

    std::unique_ptr<juce::AudioProcessor> createCustomPluginInstance(
        const juce::PluginDescription& desc,
        double sampleRate,
        int blockSize)
    {
        if (desc.name == PolyphonicOctaver::getPluginDescription().name)
        {
            return std::make_unique<PolyphonicOctaver>();
        }

        DBG("Unknown custom plugin: " + desc.name);
        throw std::invalid_argument("Unknown custom plugin: " + desc.name.toStdString());
    }

    std::unique_ptr<juce::AudioProcessor> createPluginInstance(
        const juce::PluginDescription& desc,
        double sampleRate,
        int blockSize)
    {
        if (desc.fileOrIdentifier.startsWith("notrac.internal"))
        {
            return createCustomPluginInstance(desc, sampleRate, blockSize);
        }

        juce::String error;

        auto plugin = pluginFormatManager.createPluginInstance(
            desc,
            sampleRate,
            blockSize,
            error);

        if (!plugin)
        {
            DBG("Failed to create plugin instance: " + error);
            throw std::runtime_error("Failed to create plugin instance: " + error.toStdString());
        }

        return plugin;
    }

    juce::KnownPluginList& getKnownPluginList() noexcept { return knownPluginList; }
    const juce::KnownPluginList& getKnownPluginList() const noexcept { return knownPluginList; }

    juce::AudioPluginFormatManager& getPluginFormatManager() noexcept { return pluginFormatManager; }
    const juce::AudioPluginFormatManager& getPluginFormatManager() const noexcept { return pluginFormatManager; }
};
    
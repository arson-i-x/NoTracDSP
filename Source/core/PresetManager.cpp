#include "PresetManager.h"

PresetManager::PresetManager()
{
    JUCE_ASSERT_MESSAGE_MANAGER_IS_LOCKED; // Ensure this is called on the main thread

    presetDirectory = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                          .getChildFile("NoTracDSP")
                          .getChildFile("Presets");

    if (!presetDirectory.exists())
    {
        presetDirectory.createDirectory();
    }

    loadPresets();
}

PresetManager::~PresetManager()
{
}

void PresetManager::loadPresets()
{
    presets.clear();

    juce::Array<juce::File> presetFiles =
        presetDirectory.findChildFiles(
            juce::File::findFiles,
            false,
            "*.preset");

    for (const auto &file : presetFiles)
    {
        presets.push_back(file.getFileNameWithoutExtension());
    }

    sendChangeMessage(); // Notify listeners about the update
}

Status PresetManager::savePreset(const juce::String &fileName, const juce::ValueTree &saveState)
{
    juce::File outputFile = presetDirectory.getChildFile(fileName + ".preset");

    if (!outputFile.existsAsFile())
    {
        outputFile.create();
    }

    if (auto xml = saveState.createXml())
    {
        if (xml->writeTo(outputFile))
            return Status::success();
    }

    sendChangeMessage(); // Notify listeners about the update

    return Status::failure("Failed to save preset.");
}

Status PresetManager::deletePreset(const juce::File &preset)
{
    if (preset.exists())
        preset.deleteFile();
    else
        return Status::failure("Preset file does not exist.");

    loadPresets(); // Reload presets to update the list after deletion

    sendChangeMessage(); // Notify listeners about the update

    return Status::success();
}

std::map<juce::String, Status> PresetManager::setCurrentPreset(juce::ValueTree presetState)
{
    JUCE_ASSERT_MESSAGE_MANAGER_IS_LOCKED; // Ensure this is called on the main thread

    if (!presetState.hasType("NoTracPreset"))
        throw std::invalid_argument("Invalid preset state: missing 'NoTracPreset' type.");

    auto chain = presetState.getChildWithName("PluginChain");

    if (!chain.isValid())
        throw std::invalid_argument("Invalid preset state: missing 'PluginChain' child.");

    currentPresetName = presetState.getProperty("name", "init").toString();

    return onPresetChanged(presetState); // Notify listeners about the change
}

std::map<juce::String, Status> PresetManager::setCurrentPreset(int selectedIndex)
{
    JUCE_ASSERT_MESSAGE_MANAGER_IS_LOCKED; // Ensure this is called on the main thread

    if (selectedIndex < 1 || selectedIndex > static_cast<int>(presets.size()))
        throw std::invalid_argument("Invalid preset index.");

    auto preset = presets[selectedIndex];

    auto presetFile = presetDirectory.getChildFile(preset + ".preset");

    if (!presetFile.existsAsFile())
        throw std::invalid_argument("Preset file does not exist.");

    // Load the preset and notify listeners
    auto xml = juce::XmlDocument::parse(presetFile.loadFileAsString());

    if (xml == nullptr)
        throw std::invalid_argument("Failed to parse preset.");

    juce::ValueTree presetState = juce::ValueTree::fromXml(*xml);

    if (!presetState.hasType("NoTracPreset"))
        throw std::invalid_argument("Invalid preset state: missing 'NoTracPreset' type.");

    auto chain = presetState.getChildWithName("PluginChain");

    if (!chain.isValid())
        throw std::invalid_argument("Invalid preset state: missing 'PluginChain' child.");

    currentPresetName = preset;
    
    return onPresetChanged(presetState); // Notify listeners about the change
}
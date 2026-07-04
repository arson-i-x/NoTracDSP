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

PresetList<juce::String> PresetManager::getPresets() const
{
    return presets;
}

PresetManager::~PresetManager()
{
    removeAllChangeListeners();             // Remove all listeners to prevent dangling pointers
    audioEngine.removeStatusListener(this); // Remove listener when PresetManager is destroyed to prevent dangling pointers
}

void PresetManager::loadPresets()
{
    presets.clear();

    juce::Array<juce::File> presetFiles =
        presetDirectory.findChildFiles(
            juce::File::findFiles,
            false,
            "*.preset");

    for (const auto file : presetFiles)
    {
        presets.push_back(file.getFileNameWithoutExtension());
    }

    sendChangeMessage(); // Notify listeners that the preset list has changed
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

    sendChangeMessage(); // Notify listeners that the preset list has changed

    return Status::failure("Failed to save preset.");
}

Status PresetManager::deletePreset(const juce::File &preset)
{
    if (preset.exists())
        preset.deleteFile();
    else
        return Status::failure("Preset file does not exist.");

    loadPresets(); // Reload presets to update the list after deletion

    sendChangeMessage(); // Notify listeners that the preset list has changed

    return Status::success();
}

Status PresetManager::setCurrentPreset(juce::ValueTree presetState)
{
    JUCE_ASSERT_MESSAGE_MANAGER_IS_LOCKED; // Ensure this is called on the main thread

    if (!presetState.hasType("NoTracPreset"))
        return Status::failure("Invalid preset state.");

    auto chain = presetState.getChildWithName("PluginChain");

    if (!chain.isValid())
        return Status::failure("Preset has no plugin chain.");

    stateModified = false;

    currentPresetName = presetState.getProperty("name", "init").toString();

    onPresetChanged(presetState); // Notify listeners about the change

    sendChangeMessage(); // Notify listeners that the preset has changed

    return Status::success();
}

Status PresetManager::setCurrentPreset(int selectedIndex)
{
    if (selectedIndex < 1 || selectedIndex > presets.size())
        return Status::failure("Invalid preset index.");

    auto preset = presets[selectedIndex];

    auto presetFile = presetDirectory.getChildFile(preset + ".preset");

    if (!presetFile.existsAsFile())
        return Status::failure("Preset file does not exist.");

    // Load the preset and notify listeners
    auto xml = juce::XmlDocument::parse(presetFile.loadFileAsString());

    if (xml == nullptr)
        return Status::failure("Failed to parse preset.");

    stateModified = false;

    currentPresetName = preset;
    
    onPresetChanged(juce::ValueTree::fromXml(*xml)); // Notify listeners about the change

    sendChangeMessage(); // Notify listeners that the preset has changed

    return Status::success();
}

juce::ValueTree PresetManager::createPresetState(const juce::String& presetName) const
{     
    juce::ValueTree preset("NoTracPreset");
    juce::ValueTree chain("PluginChain");

    preset.setProperty("name", presetName, nullptr);

    for (const auto& plugin : audioEngine.getActivePlugins())
    {
        auto* node = audioEngine.getAudioProcessorGraph().getNodeForId(plugin.nodeId);

        if (node == nullptr || node->getProcessor() == nullptr)
            continue;

        auto* processor = node->getProcessor();

        juce::MemoryBlock state;
        processor->getStateInformation(state);

        juce::ValueTree p("Plugin");

        p.setProperty("name", plugin.name, nullptr);
        p.setProperty("identifier", plugin.desc.createIdentifierString(), nullptr);
        p.setProperty("format", plugin.desc.pluginFormatName, nullptr);
        p.setProperty("file", plugin.desc.fileOrIdentifier, nullptr);
        p.setProperty("bypassed", plugin.bypassed, nullptr);
        p.setProperty("stateBase64", state.toBase64Encoding(), nullptr);

        chain.addChild(p, -1, nullptr);
    }

    preset.addChild(chain, -1, nullptr);
    return preset;
}
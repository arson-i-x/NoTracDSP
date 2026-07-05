#include "ChangePresetCommand.h"
#include "core/AppMessageBus.h"

ChangePresetCommand::ChangePresetCommand(PresetManager& manager, int presetId, juce::ValueTree previousState)
    : presetManager(manager), oldState(std::move(previousState)), presetId(presetId)
{
}

bool ChangePresetCommand::perform()
{
    auto status = presetManager.setCurrentPreset(presetId);
    
    if (!status.ok)
    {
        DBG("Failed to change preset: " + status.error);

        AppMessageBus::getInstance().error("Error Changing Preset", status.error);

        return false;
    }

    return true;
}


bool ChangePresetCommand::undo()
{
    // Restore the old state of the preset
    auto status = presetManager.setCurrentPreset(oldState);
    
    if (!status.ok)
    {
        DBG("Failed to restore preset: " + status.error);

        AppMessageBus::getInstance().error("Error Restoring Preset", status.error);

        return false;
    }

    return true;
}

int ChangePresetCommand::getSizeInUnits()
{
    return 10; // Arbitrary size for the command, can be adjusted based on actual memory usage
}

juce::String ChangePresetCommand::getName() const
{
    return "Change Preset to ID: " + juce::String(presetId);
}

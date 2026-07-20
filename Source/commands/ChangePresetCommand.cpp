#include "ChangePresetCommand.h"

ChangePresetCommand::ChangePresetCommand(AppController& app, int presetId, juce::ValueTree previousState)
    : app(app), oldState(std::move(previousState)), presetId(presetId)
{
}

bool ChangePresetCommand::perform()
{
    auto status = app.setCurrentPreset(presetId);
    
    if (!status.ok)
    {
        DBG("Failed to change preset: " + status.error);
        return false;
    }

    return true;
}


bool ChangePresetCommand::undo()
{
    // Restore the old state of the preset
    auto status = app.setCurrentPreset(oldState);
    
    if (!status.ok)
    {
        DBG("Failed to restore preset: " + status.error);
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

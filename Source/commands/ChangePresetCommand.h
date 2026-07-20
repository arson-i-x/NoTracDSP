#pragma once

#include "core/AppController.h"
#include "commands/AppCommand.h"

class ChangePresetCommand : public AppCommand
{
private:
    AppController& app;

    juce::ValueTree oldState;
    
    int presetId;
public:
    ChangePresetCommand(
        AppController& app, 
        int presetId,
        juce::ValueTree previousState
    );

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override;
    juce::String getName() const override;
};
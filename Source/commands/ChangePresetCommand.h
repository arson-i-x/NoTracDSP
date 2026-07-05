#include "core/PresetManager.h"
#include "commands/AppCommand.h"

class ChangePresetCommand : public AppCommand
{
private:
    PresetManager& presetManager;

    juce::ValueTree oldState;
    
    int presetId;
public:
    ChangePresetCommand(
        PresetManager& manager, 
        int presetId,
        juce::ValueTree previousState
    );

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override;
    juce::String getName() const override;
};
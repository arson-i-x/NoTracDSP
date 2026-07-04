#include "SetPluginOrderCommand.h"

bool SetPluginOrderCommand::perform()
{
    auto status = audioEngine.setPluginOrder(newOrder);
    if (!status.ok)
    {
        DBG("Failed to set plugin order: " + status.error);
        return false;
    }

    return true;
}

bool SetPluginOrderCommand::undo()
{
    if (!oldOrder.has_value())
    {
        DBG("No previous order stored for undo.");
        return false;
    }

    auto status = audioEngine.setPluginOrder(*oldOrder);
    if (!status.ok)
    {
        DBG("Failed to undo plugin order: " + status.error);
        return false;
    }

    return true;
}

juce::String SetPluginOrderCommand::getName() const
{
    return "Set Plugin Order";
}

int SetPluginOrderCommand::getSizeInUnits()
{
    return 10; // Arbitrary size for the command, can be adjusted based on actual memory usage
}



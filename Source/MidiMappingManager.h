#pragma once

#include "MidiMapping.h"
#include <vector>

class MidiMappingManager
{
public:
    void clear()
    {
        mappings.clear();
    }

    void mapTriggerToBypass(MidiTrigger trigger,
                            juce::AudioProcessorGraph::NodeID nodeId)
    {
        mappings[trigger] = MidiAction {
            MidiActionType::toggleBypass,
            nodeId
        };
    }

    void removeMapping(MidiTrigger trigger)
    {
        mappings.erase(trigger);
    }

    std::optional<MidiAction> getActionForMessage(const juce::MidiMessage& msg) const
    {
        auto trigger = triggerFromMessage(msg);

        if (!trigger.has_value())
            return std::nullopt;

        auto it = mappings.find(*trigger);

        if (it == mappings.end())
            return std::nullopt;

        return it->second;
    }

    std::vector<MidiMapping> getMappings() const
    {
        std::vector<MidiMapping> result;

        for (const auto& [trigger, action] : mappings)
            result.push_back({ trigger, action });

        return result;
    }

    static std::optional<MidiTrigger> triggerFromMessage(const juce::MidiMessage& msg)
    {
        if (msg.isController())
        {
            // Only trigger on positive CC values.
            // Avoid double triggering on value 0 release messages.
            if (msg.getControllerValue() <= 0)
                return std::nullopt;

            return MidiTrigger {
                MidiTriggerType::cc,
                msg.getChannel(),
                msg.getControllerNumber()
            };
        }

        if (msg.isNoteOn())
        {
            return MidiTrigger {
                MidiTriggerType::note,
                msg.getChannel(),
                msg.getNoteNumber()
            };
        }

        return std::nullopt;
    }

private:
    std::map<MidiTrigger, MidiAction> mappings;
};
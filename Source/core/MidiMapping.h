#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include <optional>

enum class MidiTriggerType
{
    note,
    cc
};

enum class MidiActionType
{
    toggleBypass
};

struct MidiTrigger
{
    MidiTriggerType type = MidiTriggerType::cc;
    int channel = 1;      // MIDI channels 1-16
    int number = 0;       // CC number or note number

    bool operator<(const MidiTrigger& other) const
    {
        if (type != other.type)
            return type < other.type;

        if (channel != other.channel)
            return channel < other.channel;

        return number < other.number;
    }
};

struct MidiAction
{
    MidiActionType actionType = MidiActionType::toggleBypass;
    juce::AudioProcessorGraph::NodeID nodeId;
};

struct MidiMapping
{
    MidiTrigger trigger;
    MidiAction action;
};
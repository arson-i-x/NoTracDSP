#include "PolyphonicOctaver.h"

PolyphonicOctaver::PolyphonicOctaver(AudioEngine& engine)
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo())
                                     .withOutput("Output", juce::AudioChannelSet::stereo())),
      audioEngine(engine),
      parameters(*this, nullptr)
{
    // Initialize parameters
    parameters.createAndAddParameter("mixLevel", "Mix Level", "Mix Level", 
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f, nullptr, nullptr);
    parameters.createAndAddParameter("pitchShiftAmount", "Pitch Shift Amount", "Pitch Shift Amount", 
        juce::NormalisableRange<float>(-24.0f, 24.0f), 12.0f, nullptr, nullptr);

    parameters.state = juce::ValueTree("PolyphonicOctaverParameters");
}
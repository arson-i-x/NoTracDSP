#pragma once

#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>

class PolyphonicOctaver : public juce::AudioProcessor
{
public:
    PolyphonicOctaver();
    ~PolyphonicOctaver() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    static juce::PluginDescription getPluginDescription();
private:
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> mixLevel{ 1.0f };
    std::atomic<float> pitchShiftAmount{ -12.0f }; // Default to one octave down
};

inline juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PolyphonicOctaver();
}
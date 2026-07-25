#pragma once

#include "juce_core/juce_core.h"
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

    const juce::String& getDetectedNote() { return detectedNote; }

    float getDetectedPitch() const { return detectedPitch.load(); }
    
    static juce::PluginDescription getPluginDescription();
private:
    struct InternalState
    {
        double sampleRate;
        int blockSize;
    } internal;
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> mixLevel{ 1.0f };
    std::atomic<float> pitchShiftAmount{ -12.0f }; // Default to one octave down
    std::atomic<float> detectedPitch{ 0.0f }; // Store the detected pitch (in hz) for display in the editor
    
    juce::String detectedNote{ "N/A" }; // Store the detected note name for display in the editor

    void convertDetectedPitchToNoteName(float pitch, juce::String& noteName)
    {
        // Convert the detected pitch (in Hz) to a musical note name
        // This is a placeholder implementation; you can use a more accurate method if needed
        static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        int midiNote = static_cast<int>(69 + 12 * std::log2(pitch / 440.0f));
        int noteIndex = midiNote % 12;
        int octave = (midiNote / 12) - 1;
        noteName = juce::String(noteNames[noteIndex]) + juce::String(octave);
    }

    void detectPitch(const juce::AudioBuffer<float>& buffer) 
    {
        // Detect the pitch of the input audio and update the detectedPitch variable
        // Use simple FFT or autocorrelation methods for pitch detection
        // For now, we'll just set a placeholder value
        detectedPitch.store(440.0f); // Placeholder for actual pitch detection logic
        convertDetectedPitchToNoteName(440.0f, detectedNote);
    };
};

inline juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PolyphonicOctaver();
}
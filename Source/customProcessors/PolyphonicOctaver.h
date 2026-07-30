#pragma once

#include "juce_core/juce_core.h"
#include <atomic>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

class PolyphonicOctaver : public juce::AudioProcessor,
                          public juce::ChangeBroadcaster
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

    juce::String getDetectedNote() const;

    float getDetectedPitch() const { return detectedPitch.load(); }
    float getDetectionConfidence() const { return detectionConfidence.load(); }
    bool isPitchLocked() const { return getDetectedPitch() > 0.0f && getDetectionConfidence() >= detectionConfidenceThreshold; }
    
    static juce::PluginDescription getPluginDescription();
private:
    static constexpr int analysisWindowSize = 1024;
    static constexpr float minimumDetectedFrequency = 0.0f;
    static constexpr float maximumDetectedFrequency = 10000.0f;
    static constexpr float inputLevelThreshold = 0.01f;
    static constexpr float detectionConfidenceThreshold = 0.65f;

    struct InternalState
    {
        double sampleRate = 44100.0;
        int blockSize = 0;
        int analysisWritePosition = 0;
        int consecutiveMisses = 0;
        float smoothedPitchHz = 0.0f;
    } internal;

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> mixLevel{ 1.0f };
    std::atomic<float> pitchShiftAmount{ -12.0f }; // Default to one octave down
    std::atomic<float> detectedPitch{ 0.0f }; // Store the detected pitch (in hz) for display in the editor
    std::atomic<float> detectionConfidence{ 0.0f };

    std::vector<float> analysisBuffer;
    mutable juce::SpinLock detectedStateLock;
    
    juce::String detectedNote{ "N/A" }; // Store the detected note name for display in the editor

    void convertDetectedPitchToNoteName(float pitch, juce::String& noteName);
    void pushNextSampleIntoAnalysisBuffer(float sample) noexcept;
    float detectPitchFromAnalysisBuffer(std::atomic<float>& confidence) const;
    void setDetectedState(float pitch, float confidence, const juce::String& noteName);
};

inline juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PolyphonicOctaver();
}
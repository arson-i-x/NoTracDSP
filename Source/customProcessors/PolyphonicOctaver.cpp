#include "PolyphonicOctaver.h"
#include "PolyphonicOctaverEditor.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <numeric>

PolyphonicOctaver::PolyphonicOctaver()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo())
                                     .withOutput("Output", juce::AudioChannelSet::stereo())),
      parameters(*this, nullptr)
{
    // Initialize parameters
    parameters.createAndAddParameter("mixLevel", "Mix Level", "Mix Level", 
        juce::NormalisableRange<float>(0.0f, 1.0f), mixLevel.load(), nullptr, nullptr);
    parameters.createAndAddParameter("pitchShiftAmount", "Pitch Shift Amount", "Pitch Shift Amount", 
        juce::NormalisableRange<float>(-24.0f, 24.0f), pitchShiftAmount.load(), nullptr, nullptr);

    parameters.state = juce::ValueTree("PolyphonicOctaverParameters");
}

PolyphonicOctaver::~PolyphonicOctaver() {}

void PolyphonicOctaver::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    internal.sampleRate = sampleRate;
    internal.blockSize = samplesPerBlock;
    internal.analysisWritePosition = 0;
    internal.consecutiveMisses = 0;
    internal.smoothedPitchHz = 0.0f;
    analysisBuffer.assign(analysisWindowSize, 0.0f);
    setDetectedState(0.0f, 0.0f);

    // Prepare any resources needed for processing
}

void PolyphonicOctaver::releaseResources()
{
    // Release any resources allocated in prepareToPlay
    analysisBuffer.clear();
    internal.analysisWritePosition = 0;
    internal.consecutiveMisses = 0;
    internal.smoothedPitchHz = 0.0f;
}

void PolyphonicOctaver::convertDetectedPitchToNoteName(float pitch, juce::String& noteName)
{
    // Convert the detected pitch (in Hz) to a musical note name
    // This is a simple implementation and may not cover all edge cases
    if (pitch <= 0.0f || !std::isfinite(pitch))
    {
        noteName = "N/A";
        return;
    }

    const auto midi = juce::jlimit(0, 127,
        juce::roundToInt(69.0 + 12.0 * std::log2(pitch / 440.0)));

    noteName = juce::MidiMessage::getMidiNoteName(midi, true, true, 3);
}

void PolyphonicOctaver::pushNextSampleIntoAnalysisBuffer(float sample) noexcept
{
    if (analysisBuffer.empty())
        return;

    analysisBuffer[static_cast<size_t>(internal.analysisWritePosition)] = sample;
    internal.analysisWritePosition = (internal.analysisWritePosition + 1) % analysisWindowSize;
}

float PolyphonicOctaver::detectPitchFromAnalysisBuffer(float confidence) const
{
    if (analysisBuffer.size() != static_cast<size_t>(analysisWindowSize) || internal.sampleRate <= 0.0)
        return 0.0f;
 
    // Create a copy of the analysis buffer to work with, ensuring the most recent sample is at the end
    std::vector<float> orderedSamples(static_cast<size_t>(analysisWindowSize));
    const auto writePosition = static_cast<size_t>(internal.analysisWritePosition);

    // Reorder the analysis buffer so that the most recent sample is at the end
    for (int index = 0; index < analysisWindowSize; ++index)
        orderedSamples[static_cast<size_t>(index)] = analysisBuffer[(writePosition + static_cast<size_t>(index)) % analysisBuffer.size()];

    // Accumulate the mean samples to determine if the signal is strong enough for pitch detection
    const float mean = std::accumulate(orderedSamples.begin(), orderedSamples.end(), 0.0f)
                     / static_cast<float>(orderedSamples.size());

    // Calculate the RMS (root mean square) of the samples to determine the signal strength
    float rms = 0.0f;
    for (auto& sample : orderedSamples)
    {
        sample -= mean;
        rms += sample * sample;
    }

    // Normalize the RMS value and check against the input level threshold
    rms = std::sqrt(rms / static_cast<float>(orderedSamples.size()));
    if (rms < inputLevelThreshold)
        return 0.0f;

    // Use the autocorrelation method to detect the pitch from the analysis buffer
    const int minLag = std::max(1, static_cast<int>(internal.sampleRate / maximumDetectedFrequency));
    const int maxLag = std::min(analysisWindowSize / 2, static_cast<int>(internal.sampleRate / minimumDetectedFrequency));

    // A lag is the number of samples to shift the signal for correlation. The best lag corresponds to the detected pitch.
    float bestCorrelation = 0.0f;
    int bestLag = 0;

    // Iterate through possible lags to find the best correlation
    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        float numerator = 0.0f;
        float energyA = 0.0f;
        float energyB = 0.0f;

        // Calculate the normalized cross-correlation for the current lag
        for (int index = 0; index < analysisWindowSize - lag; ++index)
        {
            const auto sampleA = orderedSamples[static_cast<size_t>(index)];
            const auto sampleB = orderedSamples[static_cast<size_t>(index + lag)];
            numerator += sampleA * sampleB;
            energyA += sampleA * sampleA;
            energyB += sampleB * sampleB;
        }

        // If either energy is zero, skip this lag to avoid division by zero
        if (energyA <= 0.0f || energyB <= 0.0f)
            continue;

        // Calculate the normalized correlation for this lag and check if it's the best one found so far
        const auto normalizedCorrelation = numerator / std::sqrt(energyA * energyB);
        if (normalizedCorrelation > bestCorrelation)
        {
            bestCorrelation = normalizedCorrelation;
            bestLag = lag;
        }
    }

    // If the best lag is valid and the confidence is above the threshold, calculate the detected pitch in Hz
    if (bestLag == 0 || confidence < detectionConfidenceThreshold)
        return 0.0f;

    // Refine the pitch estimate using parabolic interpolation around the best lag
    if (bestLag > minLag && bestLag < maxLag)
    {
        auto correlationAtLag = [&orderedSamples](int lag)
        {
            float numerator = 0.0f;
            float energyA = 0.0f;
            float energyB = 0.0f;

            // Calculate the normalized cross-correlation for the given lag
            for (int index = 0; index < analysisWindowSize - lag; ++index)
            {
                const auto sampleA = orderedSamples[static_cast<size_t>(index)];
                const auto sampleB = orderedSamples[static_cast<size_t>(index + lag)];
                numerator += sampleA * sampleB;
                energyA += sampleA * sampleA;
                energyB += sampleB * sampleB;
            }

            // If either energy is zero, return zero to avoid division by zero
            if (energyA <= 0.0f || energyB <= 0.0f)
                return 0.0f;

            // Return the normalized correlation for this lag
            return numerator / std::sqrt(energyA * energyB);
        };

        // Perform parabolic interpolation to refine the pitch estimate
        const auto left = correlationAtLag(bestLag - 1);
        const auto center = correlationAtLag(bestLag);
        const auto right = correlationAtLag(bestLag + 1);
        const auto denominator = left - (2.0f * center) + right;

        // If the denominator is not too small, calculate the offset for parabolic interpolation
        if (std::abs(denominator) > 1.0e-6f)
        {
            const auto offset = 0.5f * (left - right) / denominator;
            const auto refinedLag = static_cast<float>(bestLag) + juce::jlimit(-0.5f, 0.5f, offset);
            return static_cast<float>(internal.sampleRate) / refinedLag;
        }
    }

    // if no refinement was possible, return the pitch based on the best lag found
    return static_cast<float>(internal.sampleRate) / static_cast<float>(bestLag);
}

void PolyphonicOctaver::setDetectedState(float pitch, float confidence)
{
    detectedPitch.store(pitch);
    detectionConfidence.store(confidence);
}

void PolyphonicOctaver::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    float confidence = 0.0f;

    // Process the audio buffer and MIDI messages
    // Apply pitch shifting and mixing based on parameters
    juce::ignoreUnused(midiMessages);

    if (buffer.getNumSamples() <= 0 || buffer.getNumChannels() <= 0)
    {
        setDetectedState(0.0f, confidence);
        return;
    }

    for (int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        float monoSample = 0.0f;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            monoSample += buffer.getSample(channel, sampleIndex);
        }

        monoSample /= static_cast<float>(buffer.getNumChannels());
        pushNextSampleIntoAnalysisBuffer(monoSample);
    }

    const auto candidatePitch = detectPitchFromAnalysisBuffer(confidence);

    if (candidatePitch > 0.0f)
    {
        internal.consecutiveMisses = 0;
        internal.smoothedPitchHz = internal.smoothedPitchHz <= 0.0f
            ? candidatePitch
            : juce::jmap(0.2f, internal.smoothedPitchHz, candidatePitch);

        juce::String noteName;
        convertDetectedPitchToNoteName(internal.smoothedPitchHz, noteName);
        setDetectedState(internal.smoothedPitchHz, confidence);
        return;
    }

    if (++internal.consecutiveMisses <= 3 && internal.smoothedPitchHz > 0.0f)
    {
        juce::String noteName;
        convertDetectedPitchToNoteName(internal.smoothedPitchHz, noteName);
        setDetectedState(internal.smoothedPitchHz, 0.0f);
        return;
    }

    internal.smoothedPitchHz = 0.0f;
    setDetectedState(0.0f, 0.0f);

    sendChangeMessage(); // Notify listeners that the detected state has changed
}

juce::AudioProcessorEditor* PolyphonicOctaver::createEditor()
{
    // Return a pointer to the editor component for this processor
    return new PolyphonicOctaverEditor(*this); // Replace with actual editor if needed
}

bool PolyphonicOctaver::hasEditor() const
{
    return true;
}

const juce::String PolyphonicOctaver::getName() const
{
    return "Polyphonic Octaver";
}

bool PolyphonicOctaver::acceptsMidi() const
{
    return false; // This processor accepts MIDI input
}

bool PolyphonicOctaver::producesMidi() const
{
    return false; // This processor does not produce MIDI output
}

bool PolyphonicOctaver::isMidiEffect() const
{
    return false; // This is not a MIDI effect
}

double PolyphonicOctaver::getTailLengthSeconds() const
{
    return 0.0; // No tail length for this processor
}

int PolyphonicOctaver::getNumPrograms()
{
    return 1; // Only one program for this processor
}

int PolyphonicOctaver::getCurrentProgram()
{
    return 0; // Only one program, so return index 0
}

void PolyphonicOctaver::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
    // No program switching implemented
}

const juce::String PolyphonicOctaver::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {}; // No program names implemented
}

void PolyphonicOctaver::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
    // No program name changing implemented
}

void PolyphonicOctaver::getStateInformation(juce::MemoryBlock& destData)
{
    // Save the state of the processor (parameters) to destData
    juce::MemoryOutputStream stream(destData, true);
    parameters.state.writeToStream(stream);
}

void PolyphonicOctaver::setStateInformation(const void* data, int sizeInBytes)
{
    // Restore the state of the processor (parameters) from data
    juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
    parameters.state = juce::ValueTree::readFromStream(stream);
}

juce::PluginDescription PolyphonicOctaver::getPluginDescription() 
{
        juce::PluginDescription desc;
        desc.name = "Polyphonic Octaver";
        desc.descriptiveName = "Polyphonic Octaver";
        desc.pluginFormatName = "Internal";
        desc.manufacturerName = "NoTrac";
        desc.fileOrIdentifier = "notrac.internal.PolyphonicOctaver";
        desc.numInputChannels = 1;
        desc.numOutputChannels = 1;
        desc.uniqueId = 4770707; // unique ID for the plugin, can be set to any value
        desc.version = "1.0.0";
        desc.manufacturerName = "NoTracDSP";
        desc.category = "Effect";
        return desc;
}

// Editor UI for the PolyphonicOctaver plugin
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PolyphonicOctaver.h"
#include "juce_events/juce_events.h"

class PolyphonicOctaverEditor : public juce::AudioProcessorEditor,
                                private juce::ChangeListener
{
public:
    PolyphonicOctaverEditor(PolyphonicOctaver& processor)
        : AudioProcessorEditor(&processor), audioProcessor(processor)
    {
        audioProcessor.addChangeListener(this);
        // Set the size of the editor window
        setSize(400, 300);

        // Initialize and add UI components here (e.g., sliders, buttons)
        // For now Just initialize the editor with a simple label which is updated
        // to show the current input pitch detected by the plugin. 
        // This will be updated in the processBlock method of the plugin.
        detectedPitchLabel.setText("Detected Pitch: N/A", juce::dontSendNotification);
        detectedPitchLabel.setJustificationType(juce::Justification::centred);
        detectedPitchLabel.setFont(juce::Font{juce::FontOptions(16.0f, juce::Font::bold)});
        addAndMakeVisible(detectedPitchLabel);

        detectedNoteLabel.setText("Detected Note: N/A", juce::dontSendNotification);
        detectedNoteLabel.setJustificationType(juce::Justification::centred);
        detectedNoteLabel.setFont(juce::Font{juce::FontOptions(16.0f, juce::Font::bold)});
        addAndMakeVisible(detectedNoteLabel);

        lockStateLabel.setText("Tracking: Searching", juce::dontSendNotification);
        lockStateLabel.setJustificationType(juce::Justification::centred);
        lockStateLabel.setFont(juce::Font{juce::FontOptions(14.0f, juce::Font::plain)});
        addAndMakeVisible(lockStateLabel);

        startTimerHz(15);
    }

    ~PolyphonicOctaverEditor() override 
    {
        audioProcessor.removeChangeListener(this);
        audioProcessor.editorBeingDeleted(this);
    }

    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        if (source == &audioProcessor)
        {
            // Update the UI based on changes in the audio processor
            const auto pitch = audioProcessor.getDetectedPitch();
            const auto confidence = audioProcessor.getDetectionConfidence();

            detectedPitchLabel.setText(
                pitch > 0.0f ? "Detected Pitch: " + juce::String(pitch, 2) + " Hz"
                             : "Detected Pitch: N/A",
                juce::dontSendNotification);
            detectedNoteLabel.setText("Detected Note: " + audioProcessor.getDetectedNote(), juce::dontSendNotification);
            lockStateLabel.setText(
                audioProcessor.isPitchLocked()
                    ? "Tracking: Locked (confidence " + juce::String(confidence, 2) + ")"
                    : "Tracking: Searching",
                juce::dontSendNotification);
        }
    }

    void paint(juce::Graphics& g) override
    {
        // g.fillAll(juce::Colours::black);
        // g.setColour(juce::Colours::white);
        // g.setFont(15.0f);
        // g.drawFittedText("Polyphonic Octaver", getLocalBounds(), juce::Justification::centred, 1);
    }

    void resized() override
    {
        // Position and size UI components here
        detectedPitchLabel.setBounds(10, 40, getWidth() - 20, 20);
        detectedNoteLabel.setBounds(10, 70, getWidth() - 20, 20);
        lockStateLabel.setBounds(10, 100, getWidth() - 20, 20);
    }

private:
    juce::Label detectedPitchLabel;
    juce::Label detectedNoteLabel;
    juce::Label lockStateLabel;
    PolyphonicOctaver& audioProcessor;
};

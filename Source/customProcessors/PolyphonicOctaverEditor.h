// Editor UI for the PolyphonicOctaver plugin
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PolyphonicOctaver.h"

class PolyphonicOctaverEditor : public juce::AudioProcessorEditor
{
public:
    PolyphonicOctaverEditor(PolyphonicOctaver& processor)
        : AudioProcessorEditor(&processor), audioProcessor(processor)
    {
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
    }

    ~PolyphonicOctaverEditor() override 
    {
        audioProcessor.editorBeingDeleted(this);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::white);
        g.setFont(15.0f);
        g.drawFittedText("Polyphonic Octaver", getLocalBounds(), juce::Justification::centred, 1);
        detectedNoteLabel.setText("Detected Note: " + audioProcessor.getDetectedNote(), juce::dontSendNotification);
        detectedPitchLabel.setText("Detected Pitch: " + juce::String(audioProcessor.getDetectedPitch()), juce::dontSendNotification);
    }

    void resized() override
    {
        // Position and size UI components here
        // For example:
        // mixLevelSlider.setBounds(10, 10, getWidth() - 20, 20);
        detectedPitchLabel.setBounds(10, 40, getWidth() - 20, 20);
        detectedNoteLabel.setBounds(10, 70, getWidth() - 20, 20);
    }

private:
    juce::Label detectedPitchLabel;
    juce::Label detectedNoteLabel;
    PolyphonicOctaver& audioProcessor;
};

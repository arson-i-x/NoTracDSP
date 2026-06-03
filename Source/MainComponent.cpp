#include "MainComponent.h"

MainComponent::MainComponent()
{
    // GUI thread: build the control surface and wire it to the audio engine.
    setOpaque (true);

    titleLabel.setText ("QuadCore Prototype", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setFont (juce::Font { juce::FontOptions (28.0f, juce::Font::bold) });
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xfff4f7fb));
    addAndMakeVisible (titleLabel);

    gainLabel.setText ("Master Gain", juce::dontSendNotification);
    gainLabel.setJustificationType (juce::Justification::centredLeft);
    gainLabel.setColour (juce::Label::textColourId, juce::Colour (0xffc7d0db));
    addAndMakeVisible (gainLabel);

    gainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 88, 24);
    gainSlider.setRange (0.0, 2.0, 0.001);
    gainSlider.setValue (1.0);
    gainSlider.onValueChange = [this]
    {
        audioEngine.setMasterGain ((float) gainSlider.getValue());
        updateGainReadout();
    };
    addAndMakeVisible (gainSlider);

    gainValueLabel.setJustificationType (juce::Justification::centredRight);
    gainValueLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8bd3ff));
    addAndMakeVisible (gainValueLabel);

    deviceTypeLabel.setJustificationType (juce::Justification::centredLeft);
    deviceNameLabel.setJustificationType (juce::Justification::centredLeft);
    deviceFormatLabel.setJustificationType (juce::Justification::centredLeft);
    deviceChannelLabel.setJustificationType (juce::Justification::centredLeft);
    deviceLatencyLabel.setJustificationType (juce::Justification::centredLeft);
    deviceStatusLabel.setJustificationType (juce::Justification::centredLeft);

    for (auto* label : { &deviceTypeLabel, &deviceNameLabel, &deviceFormatLabel, &deviceChannelLabel, &deviceLatencyLabel, &deviceStatusLabel })
    {
        label->setColour (juce::Label::textColourId, juce::Colour (0xffdbe4ee));
        addAndMakeVisible (label);
    }

    deviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent> (audioEngine.getAudioDeviceManager(),
                                                                           2,
                                                                           256,
                                                                           2,
                                                                           256,
                                                                           false,
                                                                           false,
                                                                           true,
                                                                           false);
    deviceSelector->setColour (juce::Label::textColourId, juce::Colour (0xffdbe4ee));
    deviceSelector->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff243041));
    deviceSelector->setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff3f536d));
    addAndMakeVisible (deviceSelector.get());

    audioEngine.addStatusListener (this);

    updateDeviceLabels();
    updateGainReadout();
    setSize (1100, 760);
}

MainComponent::~MainComponent()
{
    audioEngine.removeStatusListener (this);
}

void MainComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.fillAll (juce::Colour (0xff0b1017));

    juce::ColourGradient gradient (juce::Colour (0xff16202c), 0.0f, 0.0f,
                                   juce::Colour (0xff0b1017), 0.0f, bounds.getBottom(),
                                   false);
    g.setGradientFill (gradient);
    g.fillRoundedRectangle (bounds.reduced (10.0f), 18.0f);

    g.setColour (juce::Colour (0x1fffffff));
    g.drawRoundedRectangle (bounds.reduced (10.0f), 18.0f, 1.0f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced (28);
    auto header = area.removeFromTop (72);

    titleLabel.setBounds (header.removeFromLeft (360));
    gainValueLabel.setBounds (header.removeFromRight (96));
    gainLabel.setBounds (header.removeFromTop (22));
    gainSlider.setBounds (header.reduced (0, 6));

    area.removeFromTop (8);

    auto leftColumn = area.removeFromLeft (330);
    auto rightColumn = area;

    auto row = leftColumn.removeFromTop (28);
    deviceTypeLabel.setBounds (row);
    row = leftColumn.removeFromTop (32);
    deviceNameLabel.setBounds (row);
    row = leftColumn.removeFromTop (46);
    deviceFormatLabel.setBounds (row);
    row = leftColumn.removeFromTop (46);
    deviceChannelLabel.setBounds (row);
    row = leftColumn.removeFromTop (46);
    deviceLatencyLabel.setBounds (row);
    row = leftColumn.removeFromTop (46);
    deviceStatusLabel.setBounds (row);

    deviceSelector->setBounds (rightColumn);
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    updateDeviceLabels();
}

void MainComponent::updateDeviceLabels()
{
    const auto status = audioEngine.getDeviceStatus();

    deviceTypeLabel.setText ("Device type: " + status.deviceType, juce::dontSendNotification);
    deviceNameLabel.setText ("Device: " + status.deviceName, juce::dontSendNotification);
    deviceFormatLabel.setText (status.formatText, juce::dontSendNotification);
    deviceChannelLabel.setText (status.channelText, juce::dontSendNotification);
    deviceLatencyLabel.setText (status.latencyText, juce::dontSendNotification);
    deviceStatusLabel.setText ("Status: " + status.statusText, juce::dontSendNotification);
}

void MainComponent::updateGainReadout()
{
    gainValueLabel.setText (juce::String (gainSlider.getValue(), 2) + "x", juce::dontSendNotification);
}
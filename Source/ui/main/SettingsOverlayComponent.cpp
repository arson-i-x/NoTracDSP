#include "SettingsOverlayComponent.h"

SettingsOverlayComponent::SettingsOverlayComponent(AudioEngine& engine)
    : audioEngine(engine)
{
    audioEngine.addStatusListener(this);

    addAndMakeVisible(titleLabel);
    titleLabel.setText("Settings", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    createDeviceSelectorUI();
    createDeviceStatusUI();
    updateDeviceLabels();
    addAndMakeVisible(closeButton);
    closeButton.setButtonText("X");
    closeButton.onClick = [this]
    {
        setVisible(false);

        if (onClosed)
            onClosed();
    };
    closeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff243041));

    addAndMakeVisible(statusLabel);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    for (int i = 0; i < 8; ++i)
    {
        auto button = std::make_unique<juce::TextButton>();
        button->setButtonText("Switch " + juce::String(i + 1));

        button->onClick = [this, i]
        {
            selectedSwitchIndex = i;
            updateStatus();

            if (onSwitchSelected)
                onSwitchSelected(i + 1);
        };

        addAndMakeVisible(*button);
        switchButtons.push_back(std::move(button));
    }

    setVisible(true);
}

void SettingsOverlayComponent::createDeviceStatusUI()
{
    deviceTypeLabel.setJustificationType(juce::Justification::centredLeft);
    deviceNameLabel.setJustificationType(juce::Justification::centredLeft);
    deviceFormatLabel.setJustificationType(juce::Justification::centredLeft);
    deviceChannelLabel.setJustificationType(juce::Justification::centredLeft);
    deviceLatencyLabel.setJustificationType(juce::Justification::centredLeft);
    deviceStatusLabel.setJustificationType(juce::Justification::centredLeft);

    for (auto *label : {&deviceTypeLabel, &deviceNameLabel, &deviceFormatLabel, &deviceChannelLabel, &deviceLatencyLabel, &deviceStatusLabel})
    {
        DBG("Adding label: " + label->getText());
        label->setColour(juce::Label::textColourId, juce::Colour(0xffdbe4ee));
        addAndMakeVisible(label);
    }
}

void SettingsOverlayComponent::createDeviceSelectorUI()
    {
        deviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent>(audioEngine.getAudioDeviceManager(),
                                                                             2,
                                                                             256,
                                                                             2,
                                                                             256,
                                                                             false,
                                                                             false,
                                                                             true,
                                                                             false);
        deviceSelector->setColour(juce::Label::textColourId, juce::Colour(0xffdbe4ee));
        deviceSelector->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff243041));
        deviceSelector->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3f536d));
        addAndMakeVisible(*deviceSelector);
    }

void SettingsOverlayComponent::openForPlugin(juce::AudioProcessorGraph::NodeID nodeId,
                                             const juce::String &pluginName)
{
    targetNodeId = nodeId;
    targetPluginName = pluginName;
    selectedSwitchIndex = -1;

    updateStatus();

    setVisible(true);
    toFront(true);
}

std::optional<juce::AudioProcessorGraph::NodeID> SettingsOverlayComponent::getTargetNodeId() const
{
    return targetNodeId;
}

int SettingsOverlayComponent::getSelectedSwitchNumber() const
{
    return selectedSwitchIndex + 1;
}

void SettingsOverlayComponent::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour(0xee000000));

    auto panel = getLocalBounds().reduced(80);
    g.setColour(juce::Colour(0xff18202a));
    g.fillRoundedRectangle(panel.toFloat(), 14.0f);

    g.setColour(juce::Colour(0xff8bd3ff));
    g.drawRoundedRectangle(panel.toFloat(), 14.0f, 2.0f);
}


void SettingsOverlayComponent::changeListenerCallback(juce::ChangeBroadcaster *)
{
    updateDeviceLabels();
}

void SettingsOverlayComponent::updateDeviceLabels()
{ 
        const auto status = audioEngine.getDeviceStatus();

        deviceTypeLabel.setText("Device type: " + status.deviceType, juce::dontSendNotification);
        deviceNameLabel.setText("Device: " + status.deviceName, juce::dontSendNotification);
        deviceFormatLabel.setText(status.formatText, juce::dontSendNotification);
        deviceChannelLabel.setText(status.channelText, juce::dontSendNotification);
        deviceLatencyLabel.setText(status.latencyText, juce::dontSendNotification);
        deviceStatusLabel.setText("Status: " + status.statusText, juce::dontSendNotification);
    }

void SettingsOverlayComponent::resized()
{
    auto controlArea = getLocalBounds().reduced(40).removeFromTop(72).removeFromRight(120);
    controlArea.removeFromRight(40);
    closeButton.setBounds(controlArea);

    auto area = getLocalBounds().reduced(40);

    titleLabel.setBounds(area.removeFromTop(40));
    area.removeFromTop(12);

    statusLabel.setBounds(area.removeFromTop(32));
    area.removeFromTop(20);

    auto leftColumn = area.removeFromLeft(520);
    leftColumn.removeFromLeft(48);
    auto rightColumn = area;
    rightColumn.removeFromRight(48);

    auto addRow = [&leftColumn](juce::Label& label)
    {
        label.setBounds(leftColumn.removeFromTop(28));
        leftColumn.removeFromTop(8);
    };

    addRow(deviceTypeLabel);
    addRow(deviceNameLabel);
    addRow(deviceFormatLabel);
    addRow(deviceChannelLabel);
    addRow(deviceLatencyLabel);
    addRow(deviceStatusLabel);

    if (deviceSelector != nullptr)
        deviceSelector->setBounds(rightColumn);
}

void SettingsOverlayComponent::updateStatus()
{
    juce::String text = "Assigning plugin: " + targetPluginName;

    if (selectedSwitchIndex >= 0)
        text += "  |  Selected switch: " + juce::String(selectedSwitchIndex + 1);

    statusLabel.setText(text, juce::dontSendNotification);
}

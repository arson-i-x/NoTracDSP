#include "SettingsOverlayComponent.h"

SettingsOverlayComponent::SettingsOverlayComponent()
{
    addAndMakeVisible(titleLabel);
    titleLabel.setText("MIDI Footswitch Mapping", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    addAndMakeVisible(closeButton);
    closeButton.setButtonText("Close");
    closeButton.onClick = [this]
    {
        setVisible(false);

        if (onClosed)
            onClosed();
    };

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

    setVisible(false);
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

void SettingsOverlayComponent::resized()
{
    auto area = getLocalBounds().reduced(100);

    titleLabel.setBounds(area.removeFromTop(40));
    area.removeFromTop(20);

    statusLabel.setBounds(area.removeFromTop(40));
    area.removeFromTop(30);

    auto grid = area.removeFromTop(160);

    const int buttonW = 120;
    const int buttonH = 40;
    const int gap = 12;

    for (int i = 0; i < (int)switchButtons.size(); ++i)
    {
        int row = i / 4;
        int col = i % 4;

        int x = grid.getX() + col * (buttonW + gap);
        int y = grid.getY() + row * (buttonH + gap);

        switchButtons[i]->setBounds(x, y, buttonW, buttonH);
    }

    closeButton.setBounds(area.removeFromBottom(40).removeFromRight(120));
}

void SettingsOverlayComponent::updateStatus()
{
    juce::String text = "Assigning plugin: " + targetPluginName;

    if (selectedSwitchIndex >= 0)
        text += "  |  Selected switch: " + juce::String(selectedSwitchIndex + 1);

    statusLabel.setText(text, juce::dontSendNotification);
}

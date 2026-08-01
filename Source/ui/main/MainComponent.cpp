#include "MainComponent.h"

MidiTrigger MainComponent::midiTriggerForSoftwareSwitch(int switchNumber)
{
    return MidiTrigger{
        MidiTriggerType::cc,
        1,
        19 + switchNumber};
}

void MainComponent::createUndoRedoButtons() 
{
    auto undoImage = resources.getIcon(IconType::Undo);
    auto redoImage = resources.getIcon(IconType::Redo);
    auto settingsImage = resources.getIcon(IconType::Settings);

    undoButton.setImages(
        false, true, true,
        undoImage, 1.0f, juce::Colours::transparentBlack,
        undoImage, 0.8f, juce::Colours::transparentBlack,
        undoImage, 0.5f, juce::Colours::transparentBlack);
    redoButton.setImages(
        false, true, true,
        redoImage, 1.0f, juce::Colours::transparentBlack,
        redoImage, 0.8f, juce::Colours::transparentBlack,
        redoImage, 0.5f, juce::Colours::transparentBlack);
    settingsButton.setImages(
        false, true, true,
        settingsImage, 1.0f, juce::Colours::transparentBlack,
        settingsImage, 0.8f, juce::Colours::transparentBlack,
        settingsImage, 0.5f, juce::Colours::transparentBlack);

    undoButton.onClick = [this]
    {
        if (!app.undo())
            AppMessageBus::getInstance().warning("Nothing to undo", "There are no actions to undo.");
    };
    redoButton.onClick = [this]
    {
        if (!app.redo())
            AppMessageBus::getInstance().warning("Nothing to redo", "There are no actions to redo.");
    };
    settingsButton.onClick = [this]
    {
        settingsOverlay.setVisible(true);
    };

    addAndMakeVisible(undoButton);
    addAndMakeVisible(redoButton);
    addAndMakeVisible(settingsButton);
}

void MainComponent::createMasterGainUI()
{
    gainLabel.setText("Master Gain", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centredLeft);
    gainLabel.setColour(juce::Label::textColourId, juce::Colour(0xffc7d0db));
    addAndMakeVisible(gainLabel);
    gainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 88, 24);
    gainSlider.setRange(0.0, 2.0, 0.001);
    gainSlider.setValue(1.0);
    gainSlider.onValueChange = [this]
    {
        app.setMasterGain((float)gainSlider.getValue());
        updateGainReadout();
    };
    addAndMakeVisible(gainSlider);
    gainValueLabel.setJustificationType(juce::Justification::centredRight);
    gainValueLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8bd3ff));
    addAndMakeVisible(gainValueLabel);
}

void MainComponent::createCustomFXUI()
{

}

void MainComponent::createTitleLabel()
{
    titleLabel.setText("QuadCore Prototype", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setFont(juce::Font{juce::FontOptions(28.0f, juce::Font::bold)});
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff4f7fb));
    addAndMakeVisible(titleLabel);
}

void MainComponent::createSettingsOverlay()
{
    addAndMakeVisible(settingsOverlay);

    // settingsOverlay.onSwitchSelected =
    //     [this](int switchNumber)
    // {
    //     auto target = settingsOverlay.getTargetNodeId();

    //     if (!target.has_value())
    //         return;

    //     MidiTrigger trigger = midiTriggerForSoftwareSwitch(switchNumber);

    //     midiMappingManager.mapTriggerToBypass(trigger, *target);

    //     DBG("Mapped switch " + juce::String(switchNumber) + " to plugin node " + juce::String(target->uid));
    // };
}


void MainComponent::closePluginListWindow()
{
    sidebarOpened = false;
    pluginListBoxComponent.setVisible(false);
    resized();
}

void MainComponent::openPluginListWindow()
{
    sidebarOpened = true;

    pluginListBoxComponent.onPluginChosen = 
    [this](const juce::PluginDescription& desc)
    {
        app.requestAddPlugin(desc);
    };

    addAndMakeVisible(pluginListBoxComponent);

    resized();
}

void MainComponent::createPluginListButton()
{
    openPluginListWindowButton.setButtonText("+");

    openPluginListWindowButton.onClick = [this]
    { 
        if (sidebarOpened)
        {
            closePluginListWindow();
        }
        else
        {
            openPluginListWindow();
        }
    };

    addAndMakeVisible(openPluginListWindowButton);
}

void MainComponent::createPluginGraphViewWithCallbacks()
{
    addAndMakeVisible(pluginGraphViewComponent);

    pluginGraphViewComponent.onPluginDoubleClicked =
        [this](juce::AudioProcessorGraph::NodeID nodeId)
    { showPluginWindow(nodeId); };

    pluginGraphViewComponent.onOrderChanged =
        [this](const std::vector<juce::AudioProcessorGraph::NodeID> &newOrder)
    { app.requestOrderChange(newOrder); };

    pluginGraphViewComponent.onPluginBypassed =
        [this](juce::AudioProcessorGraph::NodeID nodeId)
    { app.requestBypassPlugin(nodeId); };

    pluginGraphViewComponent.onPluginRemoved =
        [this](juce::AudioProcessorGraph::NodeID nodeId)
    {
        pluginWindows[nodeId]->setVisible(false);
        pluginWindows.erase(nodeId);
        app.requestRemovePlugin(nodeId); 
    };

    pluginGraphViewComponent.onMidiMapRequested =
        [this](juce::AudioProcessorGraph::NodeID nodeId)
    { app.requestMidiMapPlugin(nodeId); };
}

void MainComponent::openFirstMidiInput()
{
    devices = juce::MidiInput::getAvailableDevices();

    if (devices.isEmpty())
    {
        DBG("No MIDI input devices found.");
        return;
    }

    // For now: use first available MIDI input.
    // Later, expose this in SettingsOverlayComponent.
    auto device = devices[0];

    midiInput = juce::MidiInput::openDevice(device.identifier, this);

    if (midiInput == nullptr)
    {
        DBG("Failed to open MIDI input: " + device.name);
        return;
    }

    midiInput->start();

    DBG("Opened MIDI input: " + device.name);
}

void MainComponent::showPluginWindow(juce::AudioProcessorGraph::NodeID nodeId)
{
    if (pluginWindows[nodeId] == nullptr)
    {
        pluginWindows[nodeId] = app.getPluginWindowForNode(nodeId);
    }

    pluginWindows[nodeId]->setVisible(true);
    pluginWindows[nodeId]->toFront(true);
}

void MainComponent::paint(juce::Graphics &g)
{
    auto bounds = getLocalBounds().toFloat();

    g.fillAll(juce::Colour(0xff0b1017));

    juce::ColourGradient gradient(juce::Colour(0xff16202c), 0.0f, 0.0f,
                                  juce::Colour(0xff0b1017), 0.0f, bounds.getBottom(),
                                  false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds.reduced(10.0f), 18.0f);

    g.setColour(juce::Colour(0x1fffffff));
    g.drawRoundedRectangle(bounds.reduced(10.0f), 18.0f, 1.0f);
}

void MainComponent::resized()
{
    auto mainArea = getLocalBounds().reduced(28);

    auto titleSize = 48;
    auto titleArea = mainArea.removeFromTop(titleSize);
    titleLabel.setBounds(titleArea);

    auto gapBetweenButtons = 10;
    auto buttonSize = undoButton.getNormalImage().getWidth()/3;
    auto controlPanelArea = mainArea.removeFromTop(buttonSize + gapBetweenButtons);
    undoButton.setBounds(controlPanelArea.removeFromLeft(buttonSize+gapBetweenButtons));
    redoButton.setBounds(controlPanelArea.removeFromLeft(buttonSize+gapBetweenButtons));
    settingsButton.setBounds(controlPanelArea.removeFromLeft(buttonSize+gapBetweenButtons));

    auto sidebarArea = mainArea.removeFromLeft(64);
    auto sidebarButtonArea = sidebarArea.reduced(10).removeFromTop(64+400).removeFromBottom(64);
    openPluginListWindowButton.setBounds(sidebarButtonArea);
    if (sidebarOpened)
    {
        auto pluginListComponentArea = mainArea.removeFromLeft(300);
        pluginListBoxComponent.setBounds(pluginListComponentArea);
    } 

    mainArea.removeFromTop(12);

    // Hide the plugin list button for now, as the plugin list is now integrated into the main UI.
    // auto addPluginButtonArea = mainArea.removeFromTop(32);
    // openPluginListWindowButton.setBounds(addPluginButtonArea);

    // Hide the plugin graph view button for now, as the plugin graph view is always visible.
    // auto showPluginGraphViewButtonArea = mainArea.removeFromTop (32);
    // showPluginGraphViewButton.setBounds (showPluginGraphViewButtonArea);

    // create a preset manager box at the top of the main area
    auto presetManagerArea = mainArea.removeFromTop(80);
    presetManagerComponent.setBounds(presetManagerArea);


    // create an mainArea below the device status labels for the delay and gain controls
    auto controlArea = mainArea.removeFromTop(250);
    pluginGraphViewComponent.setBounds(controlArea);

    // auto delayTimeArea = controlArea.removeFromTop (48);
    // delayTimeLabel.setBounds (delayTimeArea.removeFromLeft (150));
    // delayTimeSlider.setBounds (delayTimeArea.removeFromLeft (200));
    // delayTimeValueLabel.setBounds (delayTimeArea.removeFromLeft (80));

    // auto delayMixArea = controlArea.removeFromTop (48);
    // delayMixLabel.setBounds (delayMixArea.removeFromLeft (150));
    // delayMixSlider.setBounds (delayMixArea.removeFromLeft (200));
    // delayMixValueLabel.setBounds (delayMixArea.removeFromLeft (80));

    // auto gainArea = controlArea.removeFromTop (48);
    // gainLabel.setBounds (gainArea.removeFromLeft (150));
    // gainSlider.setBounds (gainArea.removeFromLeft (200));
    // gainValueLabel.setBounds (gainArea.removeFromLeft (80));

    settingsOverlay.setBounds(getLocalBounds());
    settingsOverlay.toFront(false);
    settingsOverlay.setVisible(false);
}

void MainComponent::updateGainReadout()
{
    gainValueLabel.setText(juce::String(gainSlider.getValue(), 2) + "x", juce::dontSendNotification);
}

void MainComponent::updateDelayReadout()
{
    delayTimeValueLabel.setText(juce::String(delayTimeSlider.getValue(), 2) + " ms", juce::dontSendNotification);
    delayMixValueLabel.setText(juce::String(delayMixSlider.getValue(), 2) + " %", juce::dontSendNotification);
}

void MainComponent::startMidiLearnForPlugin(juce::AudioProcessorGraph::NodeID nodeId)
{
    midiLearnTarget = nodeId;
    DBG("MIDI learn armed for node: " + juce::String(nodeId.uid));
}

void MainComponent::handleIncomingMidiMessage(juce::MidiInput *,
                                              const juce::MidiMessage &message)
{
    if (midiLearnTarget.has_value())
    {
        auto trigger = MidiMappingManager::triggerFromMessage(message);

        if (trigger.has_value())
        {
            midiMappingManager.mapTriggerToBypass(*trigger, *midiLearnTarget);
            DBG("MIDI learn mapped trigger to plugin.");

            midiLearnTarget.reset();
            return;
        }
    }

    auto action = midiMappingManager.getActionForMessage(message);

    if (!action.has_value())
        return;

    if (action->actionType == MidiActionType::toggleBypass)
    {
        app.togglePluginBypass(action->nodeId);
    }
}
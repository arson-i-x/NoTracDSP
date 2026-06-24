#include "./MainComponent.h"

MidiTrigger MainComponent::midiTriggerForSoftwareSwitch(int switchNumber)
{
    return MidiTrigger{
        MidiTriggerType::cc,
        1,
        19 + switchNumber};
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
        audioEngine.setMasterGain((float)gainSlider.getValue());
        updateGainReadout();
    };
    addAndMakeVisible(gainSlider);
    gainValueLabel.setJustificationType(juce::Justification::centredRight);
    gainValueLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8bd3ff));
    addAndMakeVisible(gainValueLabel);
}

void MainComponent::createDeviceStatusUI()
{
    deviceTypeLabel.setJustificationType(juce::Justification::centredLeft);
    deviceNameLabel.setJustificationType(juce::Justification::centredLeft);
    deviceFormatLabel.setJustificationType(juce::Justification::centredLeft);
    deviceChannelLabel.setJustificationType(juce::Justification::centredLeft);
    deviceLatencyLabel.setJustificationType(juce::Justification::centredLeft);
    deviceStatusLabel.setJustificationType(juce::Justification::centredLeft);

    for (auto *label : {&deviceTypeLabel, &deviceNameLabel, &deviceFormatLabel, &deviceChannelLabel, &deviceLatencyLabel, &deviceStatusLabel})
    {
        label->setColour(juce::Label::textColourId, juce::Colour(0xffdbe4ee));
        addAndMakeVisible(label);
    }
}

void MainComponent::createCustomFXUI()
{
    delayTimeLabel.setText("Delay Time", juce::dontSendNotification);
    delayTimeLabel.setJustificationType(juce::Justification::centredLeft);
    delayTimeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffc7d0db));
    addAndMakeVisible(delayTimeLabel);
    delayTimeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    delayTimeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 88, 24);
    delayTimeSlider.setRange(0.0, 2000.0, 1.0);
    delayTimeSlider.setValue(1000.0);
    delayTimeSlider.onValueChange = [this]
    {
        audioEngine.setDelayTimeMs((float)delayTimeSlider.getValue());
        updateDelayReadout();
    };
    addAndMakeVisible(delayTimeSlider);
    delayTimeValueLabel.setJustificationType(juce::Justification::centredRight);
    delayTimeValueLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8bd3ff));
    addAndMakeVisible(delayTimeValueLabel);

    delayMixLabel.setText("Delay Mix", juce::dontSendNotification);
    delayMixLabel.setJustificationType(juce::Justification::centredLeft);
    delayMixLabel.setColour(juce::Label::textColourId, juce::Colour(0xffc7d0db));
    addAndMakeVisible(delayMixLabel);
    delayMixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    delayMixSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 88, 24);
    delayMixSlider.setRange(0.0, 1.0, 0.001);
    delayMixSlider.setValue(0.5);
    delayMixSlider.onValueChange = [this]
    {
        audioEngine.setDelayMix((float)delayMixSlider.getValue());
        updateDelayReadout();
    };
    addAndMakeVisible(delayMixSlider);
    delayMixValueLabel.setJustificationType(juce::Justification::centredRight);
    delayMixValueLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8bd3ff));
    addAndMakeVisible(delayMixValueLabel);
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
    settingsOverlay.setVisible(false);
    settingsOverlay.onSwitchSelected =
        [this](int switchNumber)
    {
        auto target = settingsOverlay.getTargetNodeId();

        if (!target.has_value())
            return;

        MidiTrigger trigger = midiTriggerForSoftwareSwitch(switchNumber);

        midiMappingManager.mapTriggerToBypass(trigger, *target);

        DBG("Mapped switch " + juce::String(switchNumber) + " to plugin node " + juce::String(target->uid));
    };
}

void MainComponent::tryAddPluginFromList(const juce::PluginDescription &desc)
{
    auto validation = audioEngine.canAddPlugin(desc);

    if (!validation.ok)
    {
        messages.error("Cannot add plugin", validation.error);
        return;
    }

    bool performed = undoManager.perform(
        new AddPluginCommand(audioEngine, desc));

    if (!performed)
    {
        messages.error("Failed to add plugin", "An unknown error occurred while adding the plugin.");
    }
    else
    {
        messages.info("Plugin added", "Successfully added plugin: " + desc.name);
    }

    refreshGraphView();
}

void MainComponent::openPluginListWindow()
{
    pluginListWindow = std::make_unique<PluginListWindow>(messages);

    auto* pluginListSelector = pluginListWindow->getPluginListBox();

    pluginListSelector->onPluginChosen = 
    [this](const juce::PluginDescription& desc)
    {
        tryAddPluginFromList(desc);
    };
}

void MainComponent::tryChangePluginBypassStateFromGraphView(juce::AudioProcessorGraph::NodeID nodeId)
{
    auto validation = audioEngine.canFindPlugin(nodeId);

    if (!validation.ok)
    {
        messages.warning("Cannot bypass plugin", validation.error);
        return;
    }

    const bool performed = undoManager.perform(
        new BypassPluginCommand(audioEngine, nodeId));

    refreshGraphView();
}

void MainComponent::tryChangePluginOrderFromGraphView(const std::vector<juce::AudioProcessorGraph::NodeID> &newOrder)
{
    auto validation = audioEngine.canSetPluginOrder(newOrder);

    if (!validation.ok)
    {
        messages.warning("Cannot reorder plugins", validation.error);
        return;
    }

    const bool performed = undoManager.perform(
        new SetPluginOrderCommand(audioEngine, newOrder));

    refreshGraphView();
}

void MainComponent::tryRemovePluginFromGraphView(juce::AudioProcessorGraph::NodeID nodeId)
{
    auto validation = audioEngine.canFindPlugin(nodeId);

    if (!validation.ok)
    {
        messages.warning("Cannot remove plugin", validation.error);
        return;
    }

    const bool performed = undoManager.perform(
        new RemovePluginCommand(audioEngine, nodeId)
    );

    refreshGraphView();
};

void MainComponent::tryMidiMapPluginFromGraphView(juce::AudioProcessorGraph::NodeID nodeId)
{
    auto name = audioEngine.getPluginName(nodeId);
    settingsOverlay.openForPlugin(nodeId, name);
}

void MainComponent::createPluginListButton()
{
    openPluginListWindowButton.setButtonText("Load Plugins");

    openPluginListWindowButton.onClick = [this]
    { openPluginListWindow(); };

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
    { tryChangePluginOrderFromGraphView(newOrder); };

    pluginGraphViewComponent.onPluginBypassed =
        [this](juce::AudioProcessorGraph::NodeID nodeId)
    { tryChangePluginBypassStateFromGraphView(nodeId); };

    pluginGraphViewComponent.onPluginRemoved =
        [this](juce::AudioProcessorGraph::NodeID nodeId)
    { tryRemovePluginFromGraphView(nodeId); };

    pluginGraphViewComponent.onMidiMapRequested =
        [this](juce::AudioProcessorGraph::NodeID nodeId)
    { tryMidiMapPluginFromGraphView(nodeId); };
}

void MainComponent::createDeviceSelectorUI()
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
    addAndMakeVisible(deviceSelector.get());
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

void MainComponent::refreshGraphView()
{
    auto activePlugins = audioEngine.getActivePlugins();

    DBG("Refreshing graph view with " + juce::String(activePlugins.size()) + " active plugins.");

    std::vector<PluginGraphItem> items;

    int x = 40;
    int y = 80;
    int width = 140;
    int height = 60;
    int gap = 50;

    DBG("Active plugins:");
    DBG("Name | Bypassed");
    DBG("-------------------");

    for (const auto &plugin : activePlugins)
    {
        items.push_back({plugin.nodeId,
                         plugin.name,
                         juce::Rectangle<int>(x, y, width, height),
                         juce::Rectangle<int>(x + width - 20, y, 20, 20), // remove button bounds
                         plugin.bypassed});

        DBG("Plugin: " + plugin.name + ", Bypassed: " + (plugin.bypassed ? "Yes" : "No") + ")");

        x += width + gap;
    }

    pluginGraphViewComponent.setPlugins(items);
}

void MainComponent::showPluginWindow(juce::AudioProcessorGraph::NodeID nodeId)
{
    if (auto it = pluginWindows.find(nodeId); it != pluginWindows.end())
    {
        it->second->setVisible(true);
        it->second->toFront(true);
        return;
    }

    if (auto *processor = audioEngine.getProcessorForNode(nodeId))
    {
        pluginWindows[nodeId].reset();
        pluginWindows[nodeId] =
            std::make_unique<PluginWindow>(processor);

        pluginWindows[nodeId]->setVisible(true);
        pluginWindows[nodeId]->toFront(true);
    }
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
    auto area = getLocalBounds().reduced(28);
    auto titleArea = area.removeFromTop(48);

    titleLabel.setBounds(titleArea);

    area.removeFromTop(8);

    deviceSelector->setBounds(area.removeFromTop(100));

    area.removeFromTop(12);

    auto addPluginButtonArea = area.removeFromTop(32);
    openPluginListWindowButton.setBounds(addPluginButtonArea);

    // auto showPluginGraphViewButtonArea = area.removeFromTop (32);
    // showPluginGraphViewButton.setBounds (showPluginGraphViewButtonArea);

    // create an area below the device status labels for the delay and gain controls
    auto controlArea = area.removeFromTop(250);
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

    auto leftColumn = area.removeFromLeft(330);
    auto rightColumn = area;

    auto row = leftColumn.removeFromTop(28);
    deviceTypeLabel.setBounds(row);
    row = leftColumn.removeFromTop(32);
    deviceNameLabel.setBounds(row);
    row = leftColumn.removeFromTop(46);
    deviceFormatLabel.setBounds(row);
    row = leftColumn.removeFromTop(46);
    deviceChannelLabel.setBounds(row);
    row = leftColumn.removeFromTop(46);
    deviceLatencyLabel.setBounds(row);
    row = leftColumn.removeFromTop(46);
    deviceStatusLabel.setBounds(row);

    deviceSelector->setBounds(rightColumn);
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster *)
{
    updateDeviceLabels();
}

void MainComponent::updateDeviceLabels()
{
    const auto status = audioEngine.getDeviceStatus();

    deviceTypeLabel.setText("Device type: " + status.deviceType, juce::dontSendNotification);
    deviceNameLabel.setText("Device: " + status.deviceName, juce::dontSendNotification);
    deviceFormatLabel.setText(status.formatText, juce::dontSendNotification);
    deviceChannelLabel.setText(status.channelText, juce::dontSendNotification);
    deviceLatencyLabel.setText(status.latencyText, juce::dontSendNotification);
    deviceStatusLabel.setText("Status: " + status.statusText, juce::dontSendNotification);
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
        audioEngine.togglePluginBypass(action->nodeId);
        refreshGraphView();
    }
}
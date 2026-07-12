#include "MainComponent.h"

MidiTrigger MainComponent::midiTriggerForSoftwareSwitch(int switchNumber)
{
    return MidiTrigger{
        MidiTriggerType::cc,
        1,
        19 + switchNumber};
}

void MainComponent::createPresetManagerBox()
{
    presetManagerComponent.onPresetChanged = [this](const juce::ValueTree& newPreset)
    {
        audioEngine.restorePresetState(newPreset);
        refreshGraphView();
    };
    
    // Listen for changes in the audio engine state
    audioEngine.addStatusListener(&presetManagerComponent); 
    addAndMakeVisible(presetManagerComponent);
}

void MainComponent::refreshGraphView()
{
    std::vector<PluginGraphItem> pluginItems;
    const auto& activePlugins = audioEngine.getActivePluginsInfo();
    pluginItems.reserve(activePlugins.size());

    for (const auto& plugin : activePlugins)
    {
        pluginItems.push_back(PluginGraphItem{
            plugin.nodeId,
            plugin.desc,
            plugin.displayName,
            plugin.bypassed,
        });
    }

    pluginGraphViewComponent.setPlugins(pluginItems);
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
        if (undoManager.canUndo())
        {
            undoManager.undo();
            refreshGraphView();
        }
        else
        {
            AppMessageBus::getInstance().warning("Nothing to undo", "There are no actions to undo.");
        }
    };
    redoButton.onClick = [this]
    {
        if (undoManager.canRedo())
        {
            undoManager.redo();
            refreshGraphView();
        }
        else 
        {
            AppMessageBus::getInstance().warning("Nothing to redo", "There are no actions to redo.");
        }
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
        audioEngine.setMasterGain((float)gainSlider.getValue());
        updateGainReadout();
    };
    addAndMakeVisible(gainSlider);
    gainValueLabel.setJustificationType(juce::Justification::centredRight);
    gainValueLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8bd3ff));
    addAndMakeVisible(gainValueLabel);
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

void MainComponent::tryAddPluginFromList(const juce::PluginDescription &desc)
{
    auto validation = audioEngine.canAddPlugin(desc);
    auto& messages = AppMessageBus::getInstance();
    if (!validation.ok)
    {
        messages.error("Cannot add plugin", validation.error);
        return;
    }

    undoManager.beginNewTransaction("Added Plugin: " + desc.name); 

    bool performed = undoManager.perform(
        new AddPluginCommand(
            audioEngine, 
            desc
        )
    );

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
        tryAddPluginFromList(desc);
    };

    addAndMakeVisible(pluginListBoxComponent);

    resized();
}

void MainComponent::tryChangePluginBypassStateFromGraphView(juce::AudioProcessorGraph::NodeID nodeId)
{
    auto validation = audioEngine.canFindPlugin(nodeId);
    auto &messages = AppMessageBus::getInstance();
    if (!validation.ok)
    {
        messages.warning("Cannot bypass plugin", validation.error);
        return;
    }
    
    undoManager.beginNewTransaction("Changed Bypass State for Plugin: " + audioEngine.getPluginName(nodeId)); 

    const bool performed = undoManager.perform(
        new BypassPluginCommand(audioEngine, nodeId));

    if (!performed)
    {
        messages.error("Failed to change bypass state", "An unknown error occurred while changing the bypass state.");
    }

    refreshGraphView();
}

void MainComponent::tryChangePluginOrderFromGraphView(const std::vector<juce::AudioProcessorGraph::NodeID> &newOrder)
{
    auto validation = audioEngine.canSetPluginOrder(newOrder);

    auto& messages = AppMessageBus::getInstance();
    if (!validation.ok)
    {
        messages.warning("Cannot reorder plugins", validation.error);
        return;
    }

    undoManager.beginNewTransaction("Changed Plugin Order"); 

    const bool performed = undoManager.perform(
        new SetPluginOrderCommand(audioEngine, newOrder));

    if (!performed)
    {
        messages.error("Failed to change plugin order", "An error occurred while changing the plugin order.");
    }

    refreshGraphView();
}

void MainComponent::tryRemovePluginFromGraphView(juce::AudioProcessorGraph::NodeID nodeId)
{
    auto validation = audioEngine.canFindPlugin(nodeId);

    auto &messages = AppMessageBus::getInstance();
    if (!validation.ok)
    {
        messages.warning("Cannot find plugin", validation.error);
        return;
    }

    if (pluginWindows.find(nodeId) != pluginWindows.end())
    {
        auto pluginWindow = pluginWindows[nodeId].get();
        auto editor = pluginWindow->getContentComponent();
        
        editor->exitModalState(0);
        pluginWindow->exitModalState(0);

        pluginWindows[nodeId]->setVisible(false);
        pluginWindows.erase(nodeId);
    }

    undoManager.beginNewTransaction("Removed Plugin: " + audioEngine.getPluginName(nodeId)); 

    const bool performed = undoManager.perform(
        new RemovePluginCommand(audioEngine, nodeId)
    );

    if (!performed)
    {
        messages.error("Failed to remove plugin", "An unknown error occurred while removing the plugin.");
    }
    else
    {
        messages.info("Plugin removed", "Successfully removed plugin.");
    }

    refreshGraphView();
};

void MainComponent::tryMidiMapPluginFromGraphView(juce::AudioProcessorGraph::NodeID nodeId)
{
    auto name = audioEngine.getPluginName(nodeId);
    settingsOverlay.openForPlugin(nodeId, name);
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

    refreshGraphView();
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
            std::make_unique<PluginWindow>(*processor);

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
    auto sidebarButtonArea = sidebarArea.reduced(10.0f).removeFromTop(64+400).removeFromBottom(64);
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
        audioEngine.togglePluginBypass(action->nodeId);
        refreshGraphView();
    }
}
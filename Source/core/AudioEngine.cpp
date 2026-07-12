#include "AudioEngine.h"

AudioEngine::AudioEngine() : juce::AudioIODeviceCallback(),
                                                   juce::ChangeListener(),
                                                   juce::AsyncUpdater(),
                                                   juce::ChangeBroadcaster()
{
    const auto initResult = audioDeviceManager.initialise (1, 2, nullptr, true);

    juce::addDefaultFormatsToManager(pluginRegistry.getPluginFormatManager());

    if (initResult.isNotEmpty())
        deviceStatus.statusText = initResult;

    // GUI thread: listen for device changes so the UI can refresh status labels safely.
    audioDeviceManager.addChangeListener (this);
    audioDeviceManager.addAudioCallback (this);
    refreshDeviceStatus();
}

void AudioEngine::setMasterGain (float newGain) noexcept
{
    // for (auto& processor : processors)
    // {
    //     if (auto& gainProcessor = dynamic_cast<GainProcessor*>(processor))
    //     {
    //         gainProcessor->setGain (newGain);
    //     }
    // }
}

void AudioEngine::setDelayTimeMs (float ms) noexcept
{
    // for (auto& processor : processors)
    // {
    //     if (auto& delayProcessor = dynamic_cast<DelayProcessor*>(processor))
    //     {
    //         delayProcessor->setDelayTimeMs (ms);
    //     }
    // }
}

void AudioEngine::setDelayMix (float mix) noexcept
{
    // delayProcessor.setDelayMix (mix);
}

void AudioEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    processingEngine = std::make_unique<ProcessingEngine>();

    sampleRate = device != nullptr ? device->getCurrentSampleRate() : 0.0;
    blockSize = device != nullptr ? device->getCurrentBufferSizeSamples() : 0;
    numChannels = device != nullptr ? juce::jmax (1, device->getActiveOutputChannels().countNumberOfSetBits())
                                               : 1;

    graphBuffer.setSize(2, blockSize);
    graphBuffer.clear();

    processingEngine->prepare(sampleRate, blockSize, 2, 2);

    rebuildGraphConnections();
}

void AudioEngine::audioDeviceStopped()
{
    processingEngine.reset();
    sampleRate = 0.0;
    blockSize = 0;
    numChannels = 0;
}

void AudioEngine::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(context);

    for (int ch = 0; ch < numOutputChannels; ++ch)
        if (outputChannelData[ch] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);

    graphBuffer.clear();

    if (numInputChannels > 0 && inputChannelData[0] != nullptr)
    {
        // Clarett input 1 -> graph stereo input
        graphBuffer.copyFrom(0, 0, inputChannelData[0], numSamples);
        graphBuffer.copyFrom(1, 0, inputChannelData[0], numSamples);
    }

    midiBuffer.clear();

    audioProcessorGraph.processBlock(graphBuffer, midiBuffer);

    if (numOutputChannels > 0 && outputChannelData[0] != nullptr)
        juce::FloatVectorOperations::copy(
            outputChannelData[0],
            graphBuffer.getReadPointer(0),
            numSamples);

    if (numOutputChannels > 1 && outputChannelData[1] != nullptr)
        juce::FloatVectorOperations::copy(
            outputChannelData[1],
            graphBuffer.getReadPointer(1),
            numSamples);
}

void AudioEngine::changeListenerCallback (juce::ChangeBroadcaster*)
{
    triggerAsyncUpdate();
}

void AudioEngine::handleAsyncUpdate()
{
    refreshDeviceStatus();
    sendChangeMessage();
}

void AudioEngine::refreshDeviceStatus() noexcept
{
    auto newStatus = DeviceStatus{};

    if (auto* deviceType = audioDeviceManager.getCurrentDeviceTypeObject())
        newStatus.deviceType = deviceType->getTypeName();

    if (auto* device = audioDeviceManager.getCurrentAudioDevice())
    {
        newStatus.deviceName = device->getName();
        newStatus.formatText = "Sample rate: " + juce::String (device->getCurrentSampleRate(), 1)
                             + " Hz | Buffer size: " + juce::String (device->getCurrentBufferSizeSamples()) + " samples";
        newStatus.channelText = "Input: " + device->getInputChannelNames().joinIntoString (", ")
                              + " | Output: " + device->getOutputChannelNames().joinIntoString (", ");
        newStatus.latencyText = "Input latency: " + juce::String (device->getInputLatencyInSamples())
                              + " samples | Output latency: " + juce::String (device->getOutputLatencyInSamples()) + " samples";
        newStatus.statusText = "Device active";
    }
    else
    {
        newStatus.deviceName = "No audio device open";
        newStatus.formatText = "Sample rate: - | Buffer size: -";
        newStatus.channelText = "Input: - | Output: -";
        newStatus.latencyText = "Input latency: - | Output latency: -";

        if (deviceStatus.statusText.isEmpty())
            newStatus.statusText = "No audio device open";
        else
            newStatus.statusText = deviceStatus.statusText;
    }

    deviceStatus = newStatus;
}

Result<juce::AudioProcessorGraph::Node::Ptr> AudioEngine::addPlugin(
    const juce::PluginDescription& desc)
{
    auto result = pluginRegistry.createPluginInstance(desc, sampleRate, blockSize);

    if (!result.ok)
        return Result<juce::AudioProcessorGraph::Node::Ptr>::failure(result.error);

    auto node = processingEngine->getGraph().addNode(std::move(result.value));

    if (node == nullptr)
        return Result<juce::AudioProcessorGraph::Node::Ptr>::failure("Failed to add plugin node to graph.");

    pluginGraphModel.addPlugin(desc, node->nodeID);

    rebuildGraphConnections();
    return Result<juce::AudioProcessorGraph::Node::Ptr>::success(node);
}

juce::AudioProcessor* AudioEngine::getProcessorForNode(juce::AudioProcessorGraph::NodeID nodeId)
{
    if (auto node = processingEngine->getGraph().getNodeForId(nodeId))
        return node->getProcessor();

    return nullptr;
}

std::optional<juce::PluginDescription>
AudioEngine::findPluginByIdentifier(
    const juce::KnownPluginList& knownPlugins,
    const juce::String& identifier) const
{
    for (const auto& desc : knownPlugins.getTypes())
    {
        if (desc.createIdentifierString() == identifier)
            return desc;
    }

    return std::nullopt;
}

void AudioEngine::clearPlugins()
{
    pluginGraphModel.clearPlugins();
    processingEngine->getGraph().clear();
}

const std::vector<ActivePlugin>& AudioEngine::getActivePluginsInfo() const noexcept
{
    return pluginGraphModel.getActivePlugins();
}

juce::ValueTree AudioEngine::createPresetState() const
{
    juce::ValueTree preset("NoTracPreset");
    juce::ValueTree chain("PluginChain");

    preset.setProperty("version", 1, nullptr);
    
    for (const auto& plugin : pluginGraphModel.getActivePlugins())
    {
        auto* node = processingEngine->getGraph().getNodeForId(plugin.nodeId);

        if (node == nullptr || node->getProcessor() == nullptr)
            continue;

        auto* processor = node->getProcessor();

        juce::MemoryBlock state;
        processor->getStateInformation(state);

        juce::ValueTree p("Plugin");

        p.setProperty("name", plugin.displayName, nullptr);
        p.setProperty("identifier", plugin.desc.createIdentifierString(), nullptr);
        p.setProperty("format", plugin.desc.pluginFormatName, nullptr);
        p.setProperty("file", plugin.desc.fileOrIdentifier, nullptr);
        p.setProperty("bypassed", plugin.bypassed, nullptr);
        p.setProperty("stateBase64", state.toBase64Encoding(), nullptr);

        chain.addChild(p, -1, nullptr);
    }

    preset.addChild(chain, -1, nullptr);
    return preset;
}

Status AudioEngine::restorePresetState(const juce::ValueTree& preset)
{
    if (!preset.hasType("NoTracPreset"))
        return Status::failure("Invalid preset file.");

    auto chain = preset.getChildWithName("PluginChain");

    if (!chain.isValid())
        return Status::failure("Preset has no plugin chain.");

    clearPlugins();

    for (int i = 0; i < chain.getNumChildren(); ++i)
    {
        auto p = chain.getChild(i);

        const auto identifier = p["identifier"].toString();

        auto desc = findPluginByIdentifier(pluginRegistry.getKnownPluginList(), identifier);

        if (!desc.has_value())
            return Status::failure("Missing plugin: " + p["name"].toString());

        auto result = addPlugin(*desc);

        if (!result.ok)
            return Status::failure(result.error);

        auto* processor = result.value->getProcessor();

        if (processor != nullptr)
        {
            juce::MemoryBlock state;

            if (state.fromBase64Encoding(p["stateBase64"].toString()))
            {
                processor->setStateInformation(
                    state.getData(),
                    (int) state.getSize());
            }
        }

        const bool bypassed = (bool) p["bypassed"];

        setPluginBypassed(result.value->nodeID, bypassed);
    }

    rebuildGraphConnections();

    return Status::success();
}

void AudioEngine::rebuildGraphConnections()
{
    processingEngine->getGraph().clear();

    // Connect the input node to the first plugin in the chain
    if (!pluginGraphModel.getActivePlugins().empty())
    {
        auto firstPluginId = pluginGraphModel.getActivePlugins().front().nodeId;
        connectStereo(processingEngine->getInputNode()->nodeID, firstPluginId);
    }

    // Connect plugins in the order specified by the plugin graph model
    for (size_t i = 0; i < pluginGraphModel.getActivePlugins().size(); ++i)
    {
        auto currentPluginId = pluginGraphModel.getActivePlugins()[i].nodeId;

        if (i + 1 < pluginGraphModel.getActivePlugins().size())
        {
            auto nextPluginId = pluginGraphModel.getActivePlugins()[i + 1].nodeId;
            connectStereo(currentPluginId, nextPluginId);
        }
        else
        {
            // Connect the last plugin to the output node
            connectStereo(currentPluginId, processingEngine->getOutputNode()->nodeID);
        }
    }
}

Result<PluginSnapshot> AudioEngine::getPluginSnapshot(juce::AudioProcessorGraph::NodeID nodeId) const
{
    for (int i = 0; i < (int) pluginGraphModel.getActivePlugins().size(); ++i)
    {
        const auto& plugin = pluginGraphModel.getActivePlugins()[i];

        if (plugin.nodeId != nodeId)
            continue;

        auto* node = audioProcessorGraph.getNodeForId(nodeId);

        if (node == nullptr || node->getProcessor() == nullptr)
            return Result<PluginSnapshot>::failure("Failed to remove plugin: Node not found.");

        juce::MemoryBlock state;
        node->getProcessor()->getStateInformation(state);

        return Result<PluginSnapshot>::success(
            PluginSnapshot {
                plugin.desc,
                plugin.displayName,
                plugin.bypassed,
                i,
                state
            });
    }

    return Result<PluginSnapshot>::failure("Failed to remove plugin: No active plugins found.");
}

Status AudioEngine::removePlugin(juce::AudioProcessorGraph::NodeID nodeId)
{
    if (!pluginGraphModel.removePlugin(nodeId))
        return Status::failure("Failed to remove plugin: Plugin not found.");

    if (!audioProcessorGraph.removeNode(nodeId))
        return Status::failure("Failed to remove plugin: Node not found.");

    rebuildGraphConnections();
    return Status::success();
}

Status AudioEngine::canAddPlugin(const juce::PluginDescription& desc) const
{
    if (desc.name.isEmpty())
        return Status::failure("Plugin description is invalid.");

    if (desc.numInputChannels <= 0)
        return Status::failure("Plugin " + desc.name + " has no audio input channels.");

    if (desc.numOutputChannels <= 0)
        return Status::failure("Plugin " + desc.name + " has no audio output channels.");

    return Status::success();
}

const std::vector<juce::AudioProcessorGraph::NodeID> AudioEngine::getPluginOrder() const
{
    std::vector<juce::AudioProcessorGraph::NodeID> order;

    for (const auto& plugin : pluginGraphModel.getActivePlugins())
        order.push_back(plugin.nodeId);

    return order;
}

Status AudioEngine::canFindPlugin(juce::AudioProcessorGraph::NodeID nodeId) const
{
    if (audioProcessorGraph.getNodeForId(nodeId) == nullptr)
        return Status::failure("Could not find plugin node.");

    for (const auto& plugin : pluginGraphModel.getActivePlugins())
    {
        if (plugin.nodeId == nodeId)
            return Status::success();
    }

    return Status::failure("Plugin is not in the active chain.");
}

bool AudioEngine::isPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId) const
{
    for (const auto& plugin : pluginGraphModel.getActivePlugins())
    {
        if (plugin.nodeId == nodeId)
            return plugin.bypassed;
    }

    return false;
}

juce::String AudioEngine::getPluginName(juce::AudioProcessorGraph::NodeID nodeId) const
{
    for (const auto& plugin : pluginGraphModel.getActivePlugins())
    {
        if (plugin.nodeId == nodeId)
            return plugin.displayName;
    }

    return {};
}

Status AudioEngine::canSetPluginOrder(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder) const
{
    if (newOrder.size() != pluginGraphModel.getActivePlugins().size())
        return Status::failure("New order does not include all active plugins.");

    for (const auto& nodeId : newOrder)
    {
        bool found = false;

        for (const auto& plugin : pluginGraphModel.getActivePlugins())
        {
            if (plugin.nodeId == nodeId)
            {
                found = true;
                break;
            }
        }

        if (!found)
            return Status::failure("New order includes a plugin that is not in the active chain.");
    }

    return Status::success();
}

Status AudioEngine::canRestorePluginSnapshot(const PluginSnapshot& snapshot) const
{
    if (snapshot.desc.name.isEmpty())
        return Status::failure("Plugin description is invalid.");

    if (snapshot.desc.numInputChannels <= 0)
        return Status::failure("Plugin " + snapshot.desc.name + " has no audio input channels.");

    if (snapshot.desc.numOutputChannels <= 0)
        return Status::failure("Plugin " + snapshot.desc.name + " has no audio output channels.");

    return Status::success();
}

// void AudioEngine::swapPlugin(size_t indexA, size_t indexB)
// {
//     if (indexA < chainNodes.size() && indexB < chainNodes.size())
//     {
//         std::swap(chainNodes[indexA], chainNodes[indexB]);
//         rebuildGraphConnections();
//     }
// }    

Status AudioEngine::setPluginOrder(
    const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder)
{
    if (!pluginGraphModel.setPluginOrder(newOrder))
    {
        DBG("setPluginOrder: new order does not include all active plugins.");
        return Status::failure("New order does not include all active plugins.");
    }

    rebuildGraphConnections();
    return Status::success();
}

void AudioEngine::connectStereo(
    juce::AudioProcessorGraph::NodeID source,
    juce::AudioProcessorGraph::NodeID dest)
{
    const bool leftOk = audioProcessorGraph.addConnection(
        { { source, 0 }, { dest, 0 } });

    const bool rightOk = audioProcessorGraph.addConnection(
        { { source, 1 }, { dest, 1 } });

    DBG("Connect "
        + juce::String(source.uid) + " -> "
        + juce::String(dest.uid)
        + " | L=" + juce::String(leftOk ? "OK" : "FAIL")
        + " R=" + juce::String(rightOk ? "OK" : "FAIL"));
}

Result<juce::AudioProcessorGraph::Node::Ptr> AudioEngine::restorePluginSnapshot(const PluginSnapshot& snapshot)
{
    auto result = addPlugin(snapshot.desc);

    if (!result.ok)
        return Result<juce::AudioProcessorGraph::Node::Ptr>::failure(result.error);

    auto node = result.value;

    if (node == nullptr || node->getProcessor() == nullptr)
        return Result<juce::AudioProcessorGraph::Node::Ptr>::failure("Failed to restore plugin snapshot: Node not found.");

    node->getProcessor()->setStateInformation(snapshot.state.getData(), (int) snapshot.state.getSize());

    for (auto& plugin : pluginGraphModel.getActivePlugins())
    {
        if (plugin.nodeId == node->nodeID)
        {
            plugin.bypassed = snapshot.bypassed;
            plugin.displayName = snapshot.name;
            break;
        }
    }

    rebuildGraphConnections();
    return Result<juce::AudioProcessorGraph::Node::Ptr>::success(node);
}

juce::AudioProcessorGraph::Node::Ptr AudioEngine::getNodeForId(juce::AudioProcessorGraph::NodeID nodeId) const
{
    return audioProcessorGraph.getNodeForId(nodeId);
}

Status AudioEngine::togglePluginBypass(juce::AudioProcessorGraph::NodeID nodeId)
{
    const bool bypassed = isPluginBypassed(nodeId);
    return setPluginBypassed(nodeId, !bypassed);
}

Status AudioEngine::setPluginBypassed(
    juce::AudioProcessorGraph::NodeID nodeId,
    bool shouldBypass)
{
    for (auto& plugin : pluginGraphModel.getActivePlugins())
    {
        if (plugin.nodeId != nodeId)
            continue;
        plugin.bypassed = shouldBypass;

        if (auto *node = audioProcessorGraph.getNodeForId(nodeId))
            node->setBypassed(shouldBypass);
        else
            return Status::failure("Plugin node not found in audio processor graph.");
        return Status::success();
    }

    return Status::failure("Plugin not found in active plugins.");
}

void AudioEngine::shutdown()
{
    audioDeviceManager.removeAudioCallback(this);
    audioDeviceManager.removeChangeListener(this);

    pluginGraphModel.clearPlugins();
    audioProcessorGraph.clear();

    inputNode = nullptr;
    outputNode = nullptr;
}

DeviceStatus AudioEngine::getDeviceStatus() const noexcept
{
    return deviceStatus;
}
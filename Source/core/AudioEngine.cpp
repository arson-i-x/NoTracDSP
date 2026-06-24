#include "AudioEngine.h"

AudioEngine::AudioEngine(AppMessageBus& messageBus) : juce::AudioIODeviceCallback(),
                                                   juce::ChangeListener(),
                                                   juce::AsyncUpdater(),
                                                   juce::ChangeBroadcaster(),
                                                   messages(messageBus)
{
    const auto initResult = audioDeviceManager.initialise (1, 2, nullptr, true);

    juce::addDefaultFormatsToManager(pluginFormatManager);

    if (initResult.isNotEmpty())
        deviceStatus.statusText = initResult;

    // Set up the audio processing chain. The order of processors in this array determines the order in which they are applied to the audio signal.
    // processors.push_back(
    //     std::make_unique<GainProcessor>()
    // );

    // processors.push_back(
    //     std::make_unique<DelayProcessor>()
    // );

    inputNode = audioProcessorGraph.addNode(
    std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));

    outputNode = audioProcessorGraph.addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));

    // GUI thread: listen for device changes so the UI can refresh status labels safely.
    audioDeviceManager.addChangeListener (this);
    audioDeviceManager.addAudioCallback (this);
    refreshDeviceStatus();
}

AudioEngine::~AudioEngine()
{
    audioDeviceManager.removeAudioCallback (this);
    audioDeviceManager.removeChangeListener (this);
}

AudioEngine::DeviceStatus AudioEngine::getDeviceStatus() const noexcept
{
    return deviceStatus;
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

void AudioEngine::addStatusListener (juce::ChangeListener* listener)
{
    addChangeListener (listener);
}

void AudioEngine::removeStatusListener (juce::ChangeListener* listener)
{
    removeChangeListener (listener);
}

void AudioEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    sampleRate = device != nullptr ? device->getCurrentSampleRate() : 0.0;
    blockSize = device != nullptr ? device->getCurrentBufferSizeSamples() : 0;
    numChannels = device != nullptr ? juce::jmax (1, device->getActiveOutputChannels().countNumberOfSetBits())
                                               : 1;

    graphBuffer.setSize(2, blockSize);
    graphBuffer.clear();

    audioProcessorGraph.setPlayConfigDetails(2, 2, sampleRate, blockSize);
    audioProcessorGraph.prepareToPlay(sampleRate, blockSize);

    rebuildGraphConnections();
}

void AudioEngine::audioDeviceStopped()
{
    // for (auto& processor : processors)
    // {
    //     processor->prepare (0.0, 0, 0);
    // }
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

AppCommand::Result<juce::AudioProcessorGraph::Node::Ptr> AudioEngine::addPlugin(
    const juce::PluginDescription& desc,
    juce::AudioPluginFormatManager& formatManager)
{
    juce::String error;

    auto plugin = formatManager.createPluginInstance(
        desc,
        sampleRate,
        blockSize,
        error);

    if (!plugin)
    {
        DBG("Failed to load plugin: " + error);
        return AppCommand::Result<juce::AudioProcessorGraph::Node::Ptr>
                         ::failure("Failed to load plugin: " + error);
    }

    auto node = audioProcessorGraph.addNode(std::move(plugin));

    if (!node)
    {
        DBG("Failed to add plugin node to graph.");
        return AppCommand::Result<juce::AudioProcessorGraph::Node::Ptr>
                         ::failure("Failed to add plugin node to graph.");
    }

    activePlugins.push_back({
        node->nodeID,
        desc,
        desc.name,
        node->isBypassed()
    });

    rebuildGraphConnections();

    return AppCommand::Result<juce::AudioProcessorGraph::Node::Ptr>
                     ::success(node);
}

juce::AudioProcessor* AudioEngine::getProcessorForNode(juce::AudioProcessorGraph::NodeID nodeId)
{
    if (auto node = audioProcessorGraph.getNodeForId(nodeId))
        return node->getProcessor();

    return nullptr;
}

void AudioEngine::rebuildGraphConnections()
{
    if (inputNode == nullptr || outputNode == nullptr)
        return;

    auto connections = audioProcessorGraph.getConnections();

    for (const auto& connection : connections)
        audioProcessorGraph.removeConnection(connection);

    auto previousNode = inputNode->nodeID;

    for (const auto& plugin : activePlugins)
    {
        if (audioProcessorGraph.getNodeForId(plugin.nodeId) == nullptr)
            continue;

        connectStereo(previousNode, plugin.nodeId);
        previousNode = plugin.nodeId;
    }

    connectStereo(previousNode, outputNode->nodeID);
}

AppCommand::Result<AudioEngine::PluginSnapshot> AudioEngine::getPluginSnapshot(juce::AudioProcessorGraph::NodeID nodeId) const
{
    for (int i = 0; i < (int) activePlugins.size(); ++i)
    {
        const auto& plugin = activePlugins[i];

        if (plugin.nodeId != nodeId)
            continue;

        auto* node = audioProcessorGraph.getNodeForId(nodeId);

        if (node == nullptr || node->getProcessor() == nullptr)
            return AppCommand::Result<AudioEngine::PluginSnapshot>::failure("Failed to remove plugin: Node not found.");

        juce::MemoryBlock state;
        node->getProcessor()->getStateInformation(state);

        return AppCommand::Result<AudioEngine::PluginSnapshot>::success(
            PluginSnapshot {
                plugin.desc,
                plugin.name,
                plugin.bypassed,
                i,
                state
            });
    }

    return AppCommand::Result<AudioEngine::PluginSnapshot>::failure("Failed to remove plugin: No active plugins found.");
}

AppCommand::Status
AudioEngine::removePlugin(juce::AudioProcessorGraph::NodeID nodeId)
{
    activePlugins.erase(
        std::remove_if(activePlugins.begin(), activePlugins.end(),
            [nodeId](const AudioEngine::ActivePlugin& p)
            {
                return p.nodeId == nodeId;
            }),
        activePlugins.end());

    auto nodeRemoved = audioProcessorGraph.removeNode(nodeId);

    if (!nodeRemoved)
    {
        DBG("Failed to remove node with ID " + juce::String(nodeId.uid));
        return AppCommand::Status::failure("Failed to remove node with ID " + juce::String(nodeId.uid));
    }

    rebuildGraphConnections();
    return AppCommand::Status::success();
}

AppCommand::Status AudioEngine::canAddPlugin(const juce::PluginDescription& desc) const
{
    if (desc.name.isEmpty())
        return AppCommand::Status::failure("Plugin description is invalid.");

    if (desc.numInputChannels <= 0)
        return AppCommand::Status::failure("Plugin " + desc.name + " has no audio input channels.");

    if (desc.numOutputChannels <= 0)
        return AppCommand::Status::failure("Plugin " + desc.name + " has no audio output channels.");

    return AppCommand::Status::success();
}

AppCommand::Status AudioEngine::canFindPlugin(juce::AudioProcessorGraph::NodeID nodeId) const
{
    if (audioProcessorGraph.getNodeForId(nodeId) == nullptr)
        return AppCommand::Status::failure("Could not find plugin node.");

    for (const auto& plugin : activePlugins)
    {
        if (plugin.nodeId == nodeId)
            return AppCommand::Status::success();
    }

    return AppCommand::Status::failure("Plugin is not in the active chain.");
}

AppCommand::Status AudioEngine::canSetPluginOrder(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder) const
{
    if (newOrder.size() != activePlugins.size())
        return AppCommand::Status::failure("New order does not include all active plugins.");

    for (const auto& nodeId : newOrder)
    {
        bool found = false;

        for (const auto& plugin : activePlugins)
        {
            if (plugin.nodeId == nodeId)
            {
                found = true;
                break;
            }
        }

        if (!found)
            return AppCommand::Status::failure("New order includes a plugin that is not in the active chain.");
    }

    return AppCommand::Status::success();
}

AppCommand::Status AudioEngine::canRestorePluginSnapshot(const PluginSnapshot& snapshot) const
{
    if (snapshot.desc.name.isEmpty())
        return AppCommand::Status::failure("Plugin description is invalid.");

    if (snapshot.desc.numInputChannels <= 0)
        return AppCommand::Status::failure("Plugin " + snapshot.desc.name + " has no audio input channels.");

    if (snapshot.desc.numOutputChannels <= 0)
        return AppCommand::Status::failure("Plugin " + snapshot.desc.name + " has no audio output channels.");

    return AppCommand::Status::success();
}

// void AudioEngine::swapPlugin(size_t indexA, size_t indexB)
// {
//     if (indexA < chainNodes.size() && indexB < chainNodes.size())
//     {
//         std::swap(chainNodes[indexA], chainNodes[indexB]);
//         rebuildGraphConnections();
//     }
// }    

AppCommand::Status AudioEngine::setPluginOrder(
    const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder)
{
    std::vector<AudioEngine::ActivePlugin> reordered;

    for (auto nodeId : newOrder)
    {
        auto it = std::find_if(activePlugins.begin(), activePlugins.end(),
            [nodeId](const AudioEngine::ActivePlugin& p)
            {
                return p.nodeId == nodeId;
            });

        if (it != activePlugins.end())
            reordered.push_back(*it);
    }

    if (reordered.size() != activePlugins.size())
    {
        DBG("setPluginOrder: new order does not include all active plugins.");
        return AppCommand::Status::failure("New order does not include all active plugins.");
    }

    activePlugins = std::move(reordered);

    rebuildGraphConnections();
    return AppCommand::Status::success();
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

std::vector<AudioEngine::ActivePluginInfo> AudioEngine::getActivePlugins() const
{
    std::vector<AudioEngine::ActivePluginInfo> result;

    for (const auto& plugin : activePlugins)
    {
        result.push_back({
            plugin.nodeId,
            plugin.name,
            plugin.bypassed
        });
    }

    return result;
}

AppCommand::Status AudioEngine::restorePluginSnapshot(const PluginSnapshot& snapshot, juce::AudioPluginFormatManager& formatManager)
{
    auto result = addPlugin(snapshot.desc, formatManager);

    if (!result.ok)
        return AppCommand::Status::failure(result.error);

    auto node = result.value;

    if (node == nullptr || node->getProcessor() == nullptr)
        return AppCommand::Status::failure("Failed to restore plugin snapshot: Node not found.");

    node->getProcessor()->setStateInformation(snapshot.state.getData(), (int) snapshot.state.getSize());

    for (auto& plugin : activePlugins)
    {
        if (plugin.nodeId == node->nodeID)
        {
            plugin.bypassed = snapshot.bypassed;
            plugin.name = snapshot.name;
            break;
        }
    }

    rebuildGraphConnections();
    return AppCommand::Status::success();
}

juce::AudioProcessorGraph::Node::Ptr AudioEngine::getNodeForId(juce::AudioProcessorGraph::NodeID nodeId) const
{
    return audioProcessorGraph.getNodeForId(nodeId);
}

AudioEngine::ActivePluginInfo AudioEngine::getActivePluginInfo(juce::AudioProcessorGraph::NodeID nodeId) const
{
    for (const auto& plugin : activePlugins)
    {
        if (plugin.nodeId == nodeId)
        {
            return {
                plugin.nodeId,
                plugin.name,
                plugin.bypassed
            };
        }
    }

    return { juce::AudioProcessorGraph::NodeID(), "Unknown Plugin", false };
}

AudioEngine::ActivePlugin AudioEngine::getActivePlugin(juce::AudioProcessorGraph::NodeID nodeId) const
{
    for (const auto& plugin : activePlugins)
    {
        if (plugin.nodeId == nodeId)
        {
            return {
                plugin.nodeId,
                plugin.desc,
                plugin.name,
                plugin.bypassed
            };
        }
    }
    DBG("getActivePlugin: Node with ID " + juce::String(nodeId.uid) + " not found. Returning default ActivePlugin.");
    return { juce::AudioProcessorGraph::NodeID(), juce::PluginDescription(),"Unknown Plugin", false };
}

AppCommand::Status AudioEngine::togglePluginBypass(juce::AudioProcessorGraph::NodeID nodeId)
{
    const bool bypassed = isPluginBypassed(nodeId);
    return setPluginBypassed(nodeId, !bypassed);
}

AppCommand::Status AudioEngine::setPluginBypassed(
    juce::AudioProcessorGraph::NodeID nodeId,
    bool shouldBypass)
{
    for (auto& plugin : activePlugins)
    {
        if (plugin.nodeId != nodeId)
            continue;
        plugin.bypassed = shouldBypass;

        if (auto *node = audioProcessorGraph.getNodeForId(nodeId))
            node->setBypassed(shouldBypass);
        else
            return AppCommand::Status::failure("Plugin node not found in audio processor graph.");
        return AppCommand::Status::success();
    }

    return AppCommand::Status::failure("Plugin not found in active plugins.");
}

bool AudioEngine::isPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId) const
{
    for (const auto& plugin : activePlugins)
    {
        if (plugin.nodeId == nodeId)
            return plugin.bypassed;
    }

    return false;
}

juce::String AudioEngine::getPluginName(juce::AudioProcessorGraph::NodeID nodeId) const
{
    for (const auto& plugin : activePlugins)
    {
        if (plugin.nodeId == nodeId)
            return plugin.name;
    }

    return "Unknown Plugin";
}
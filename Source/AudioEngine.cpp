#include "AudioEngine.h"
#include <juce_audio_processors/juce_audio_processors.h>

AudioEngine::AudioEngine()
{
    const auto initResult = audioDeviceManager.initialise (1, 2, nullptr, true);

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

juce::AudioProcessorGraph::Node::Ptr AudioEngine::addPlugin(
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
        return nullptr;
    }

    auto node = audioProcessorGraph.addNode(std::move(plugin));

    activePlugins.push_back({
        node->nodeID,
        desc,
        desc.name,
        node->isBypassed()
    });

    rebuildGraphConnections();

    return node;
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

void AudioEngine::removePlugin(juce::AudioProcessorGraph::NodeID nodeId)
{
    activePlugins.erase(
        std::remove_if(activePlugins.begin(), activePlugins.end(),
            [nodeId](const ActivePlugin& p)
            {
                return p.nodeId == nodeId;
            }),
        activePlugins.end());

    audioProcessorGraph.removeNode(nodeId);
    rebuildGraphConnections();
}

// void AudioEngine::swapPlugin(size_t indexA, size_t indexB)
// {
//     if (indexA < chainNodes.size() && indexB < chainNodes.size())
//     {
//         std::swap(chainNodes[indexA], chainNodes[indexB]);
//         rebuildGraphConnections();
//     }
// }    

void AudioEngine::toggleBypass(juce::AudioProcessorGraph::NodeID nodeId)
{
    if (auto node = audioProcessorGraph.getNodeForId(nodeId))
    {
        bool currentlyBypassed = node->isBypassed();
        DBG("Toggling bypass for node " + juce::String(nodeId.uid) + " | Currently bypassed: " + (currentlyBypassed ? "Yes" : "No"));
        node->setBypassed(!currentlyBypassed);

        // update our activePlugins list to reflect the change
        for (auto& plugin : activePlugins)
        {
            if (plugin.nodeId == nodeId)
            {
                plugin.bypassed = !currentlyBypassed;
                return;
            }
        }
    } else {
        DBG("Node with ID " + juce::String(nodeId.uid) + " not found. Cannot toggle bypass.");
    }
}

void AudioEngine::setPluginOrder(
    const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder)
{
    std::vector<ActivePlugin> reordered;

    for (auto nodeId : newOrder)
    {
        auto it = std::find_if(activePlugins.begin(), activePlugins.end(),
            [nodeId](const ActivePlugin& p)
            {
                return p.nodeId == nodeId;
            });

        if (it != activePlugins.end())
            reordered.push_back(*it);
    }

    activePlugins = std::move(reordered);

    rebuildGraphConnections();
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

std::vector<ActivePluginInfo> AudioEngine::getActivePlugins() const
{
    std::vector<ActivePluginInfo> result;

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

void AudioEngine::setPluginBypassed(
    juce::AudioProcessorGraph::NodeID nodeId,
    bool shouldBypass)
{
    for (auto& plugin : activePlugins)
    {
        if (plugin.nodeId == nodeId)
        {
            plugin.bypassed = shouldBypass;

            if (auto* node = audioProcessorGraph.getNodeForId(nodeId))
                node->setBypassed(shouldBypass);

            return;
        }
    }
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
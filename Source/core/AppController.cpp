#include "AppController.h"

#include "commands/AddPluginCommand.h"
#include "commands/ChangePresetCommand.h"
#include "commands/RemovePluginCommand.h"
#include "commands/SetPluginOrderCommand.h"
#include "juce_data_structures/juce_data_structures.h"

AppController::AppController() : ioEngine(processingEngine)
{
    pluginGraphModel.addChangeListener(this);
    ioEngine.addChangeListener(this);
    
    // called every time the preset manager changes the current preset,
    // either by loading a new preset or changing the current preset's state
    presetManager.onPresetChanged = [this](const juce::ValueTree& newPreset)
    {
        return restorePresetState(newPreset);
    };

    initializeIO();
}

AppController::~AppController() noexcept
{
    shutdown();
}

void AppController::initializeIO()
{
    // save to a temporary preset state the current state of the plugin graph, 
    // so that if the IO initialization fails, we can restore the previous state
    juce::ValueTree tempPresetState = createPresetState();

    processingEngine.clearGraph();
    processingEngine.prepare(ioEngine.getSampleRate(), ioEngine.getBlockSize(), 2, 2);

    restorePresetState(tempPresetState);
}

void AppController::addStatusListener(juce::ChangeListener *listener)
{
    juce::MessageManager::callAsync([this, listener]() { addChangeListener(listener); });
}

void AppController::removeStatusListener(juce::ChangeListener *listener)
{
    juce::MessageManager::callAsync([this, listener]() { removeChangeListener(listener); });
}

void AppController::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &ioEngine)
    {
        // Handle device status changes from IOEngine
        // reinitialize processing engine with new sample rate and block size
        initializeIO();
    }

    if (source == &pluginGraphModel)
    {
        // Handle changes in the plugin graph model
        rebuildGraphConnections();
    }
}

void AppController::shutdown()
{
    pluginGraphModel.removeChangeListener(this);
    ioEngine.removeChangeListener(this);
    ioEngine.audioDeviceStopped();
    processingEngine.release();
}

void AppController::handleAsyncUpdate()
{
    // Handle any asynchronous updates here
    // For example, you might want to update the UI or notify listeners about changes
}

/* Runs when the graph model changes, rebuilds the connections in the processing 
engine graph according to the graph model's plugin order and connections. */
void AppController::rebuildGraphConnections()
{
    processingEngine.clearConnections();

    // if the graph model is empty, just return
    if (pluginGraphModel.getActivePlugins().empty())
        return;

    // Rebuild connections based on the plugin graph model
    const auto& orderedPlugins = pluginGraphModel.getActivePlugins();

    // Connect the first plugin to the audio input
    auto inputNode = processingEngine.getInputNode()->nodeID;
    auto outputNode = processingEngine.getOutputNode()->nodeID;
    auto currentPluginNode = orderedPlugins.front().nodeId;
    connectStereo(inputNode, currentPluginNode);

    // start from the second plugin in the ordered list, and connect each plugin to the next one
    for (int i = 1; i < (int) orderedPlugins.size(); ++i)
    {
        const auto& plugin = orderedPlugins[i];

        if (plugin.bypassed)
        {
            // If the plugin is bypassed, skip connecting it and connect the previous plugin to the next one
            continue;
        }

        connectStereo(currentPluginNode, plugin.nodeId);

        currentPluginNode = plugin.nodeId;
    }

    connectStereo(currentPluginNode, outputNode);
}

bool AppController::isPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId) const
{
    return pluginGraphModel.getActivePlugin(nodeId).bypassed;
}

void AppController::connectStereo(
    juce::AudioProcessorGraph::NodeID source,
    juce::AudioProcessorGraph::NodeID dest)
{
    const bool leftOk = processingEngine.addConnection(
        { { source, 0 }, { dest, 0 } });

    const bool rightOk = processingEngine.addConnection(
        { { source, 1 }, { dest, 1 } });

    DBG("Connect "
        + juce::String(source.uid) + " -> "
        + juce::String(dest.uid)
        + " | L=" + juce::String(leftOk ? "OK" : "FAIL")
        + " R=" + juce::String(rightOk ? "OK" : "FAIL"));
}

void AppController::requestOrderChange(
    const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder)
{
    undoManager.beginNewTransaction("Change Plugin Order");

    bool performed = undoManager.perform(new SetPluginOrderCommand(*this, newOrder));

    if (!performed)
    {
        AppMessageBus::getInstance().error(
            "Failed to change plugin order", 
            "An unknown error occurred while changing the plugin order."
        );
    }
}

Status AppController::setOrder(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder)
{
    try
    {
        pluginGraphModel.setPluginOrder(newOrder);
        return Status::success();
    }
    catch (const std::exception& e)
    {
        return Status::failure(e.what());
    }
}

void AppController::requestAddPlugin(const juce::PluginDescription& desc)
{
    undoManager.beginNewTransaction("Add Plugin: " + desc.name);

    bool performed = undoManager.perform(new AddPluginCommand(*this, desc));

    if (!performed)
    {
        AppMessageBus::getInstance().error(
            "Failed to add plugin", 
            "An unknown error occurred while adding the plugin."
        );
    }
}

void AppController::requestRemovePlugin(juce::AudioProcessorGraph::NodeID nodeId)
{
    auto pluginName = getPluginDisplayName(nodeId);

    undoManager.beginNewTransaction("Removed Plugin: " + pluginName); 

    bool performed = undoManager.perform(new RemovePluginCommand(*this, nodeId));

    if (!performed)
    {
        AppMessageBus::getInstance().error(
            "Failed to remove plugin", 
            "An unknown error occurred while removing the plugin."
        );
    }
}


Result<juce::AudioProcessorGraph::NodeID> AppController::addPlugin(
    const juce::PluginDescription& desc)
{
    // validate plugin description before attempting to create an instance
    auto validation = canAddPlugin(desc);
    if (!validation.ok)
        return Result<juce::AudioProcessorGraph::NodeID>::failure(validation.error);

    // try to create the plugin instance and add it to the graph
    try {
        // create plugin instance using the plugin registry
        auto plugin = pluginRegistry.createPluginInstance(
            desc, 
            ioEngine.getSampleRate(), 
            ioEngine.getBlockSize()
        );

        // add the plugin to the processing engine graph
        auto node = processingEngine.addNode(std::move(plugin));

        if (node == nullptr)
            return Result<juce::AudioProcessorGraph::NodeID>::failure("Failed to add plugin node to graph.");

        // if successful, add the plugin to the plugin graph model for UI representation
        if (!pluginGraphModel.addPluginDescription(desc, node->nodeID))
        {
            processingEngine.removeNode(node->nodeID);
            return Result<juce::AudioProcessorGraph::NodeID>::failure("Failed to add plugin to graph model.");
        }
      
        sendChangeMessage(); // Notify listeners that a new plugin has been added

        return Result<juce::AudioProcessorGraph::NodeID>::success(node->nodeID);
    } catch (const std::exception& e) {
        return Result<juce::AudioProcessorGraph::NodeID>::failure(e.what());
    }
}

Result<juce::AudioProcessorGraph::NodeID> AppController::addPlugin(const ActivePlugin& activePlugin)
{
    // validate plugin description before attempting to create an instance
    auto validation = canAddPlugin(activePlugin.desc);
    if (!validation.ok)
        return Result<juce::AudioProcessorGraph::NodeID>::failure(validation.error);

    // create plugin instance using the plugin registry
    auto plugin = pluginRegistry.createPluginInstance(
        activePlugin.desc,
        ioEngine.getSampleRate(), 
        ioEngine.getBlockSize()
    );

    if (!plugin)
        return Result<juce::AudioProcessorGraph::NodeID>::failure("Failed to create plugin instance");

    // add the plugin to the processing engine graph
    auto node = processingEngine.addNode(std::move(plugin));

    if (node == nullptr)
        return Result<juce::AudioProcessorGraph::NodeID>::failure("Failed to add plugin node to graph.");

    // if successful, add the plugin to the plugin graph model for UI representation
    if (!pluginGraphModel.addActivePlugin(activePlugin, node->nodeID))
    {
        processingEngine.removeNode(node->nodeID);
        return Result<juce::AudioProcessorGraph::NodeID>::failure("Failed to add plugin to graph model.");
    }

    if (activePlugin.state.has_value())
    {
        // Restore the plugin state
        if (node->getProcessor() != nullptr)
        {
            juce::MemoryBlock state;
            state.copyFrom(&activePlugin.state.value(), 0, activePlugin.state.value().getSize());
            node->getProcessor()->setStateInformation(state.getData(), (int)state.getSize());
        }
    }

    return Result<juce::AudioProcessorGraph::NodeID>::success(node->nodeID);
}

void AppController::clearPlugins()
{
    pluginGraphModel.clearPlugins();
    processingEngine.clearGraph();
}

std::unique_ptr<PluginWindow> AppController::getPluginWindowForNodeId(juce::AudioProcessorGraph::NodeID nodeId)
{
    auto node = processingEngine.getGraph().getNodeForId(nodeId);
    if (node == nullptr || node->getProcessor() == nullptr)
        return nullptr;
    return std::make_unique<PluginWindow>(*node->getProcessor());
}

void AppController::requestSetCurrentPreset(int presetId)
{
    undoManager.beginNewTransaction("Set Preset ID: " + juce::String(presetId));

    bool performed = undoManager.perform(new ChangePresetCommand(*this, presetId, createPresetState()));

    if (!performed)
    {
        AppMessageBus::getInstance().error("Failed to set preset", "An unknown error occurred while setting the preset.");
    }
}

Status AppController::setCurrentPreset(int presetId)
{
    try
    {
        auto results = presetManager.setCurrentPreset(presetId);
        for (const auto& [pluginName, status] : results)
        {
            if (!status.ok)
                return Status::failure("Failed to set preset for plugin " + pluginName + ": " + status.error);
        }
        return Status::success();
    }
    catch (const std::exception& e)
    {
        return Status::failure(e.what());
    }
}

Status AppController::setCurrentPreset(const juce::ValueTree& presetState)
{
    try 
    {
        auto results = presetManager.setCurrentPreset(presetState);
        for (const auto& [pluginName, status] : results)
        {
            if (!status.ok)
            {
                DBG("Failed to set preset for plugin " + pluginName + ": " + status.error);
                AppMessageBus::getInstance().error("Failed to set preset for plugin " + pluginName, status.error);
                return Status::failure(status.error);
            }
        }

        return Status::success();
    }
    catch (const std::exception& e)
    {
        return Status::failure(e.what());
    }
}

void AppController::requestSetCurrentPreset(const juce::ValueTree& presetState)
{
    auto status = setCurrentPreset(presetState);
    if (!status.ok)
        AppMessageBus::getInstance().error("Failed to set preset", status.error);
}

void AppController::requestSavePreset(const juce::String& presetName, const juce::ValueTree& presetState)
{
    auto status = savePreset(presetName, presetState);
    if (!status.ok)
    {
        AppMessageBus::getInstance().error("Failed to save preset", status.error);
        return;
    }
}

Status AppController::savePreset(const juce::String& presetName, const juce::ValueTree& presetState)
{
    auto status = presetManager.savePreset(presetName, presetState);
    if (!status.ok)
        return status;

    presetManager.loadPresets();
    return Status::success();
}

juce::ValueTree AppController::createPresetState() const
{
    juce::ValueTree preset("NoTracPreset");
    juce::ValueTree chain("PluginChain");

    preset.setProperty("version", 1, nullptr);
    
    for (const auto& plugin : pluginGraphModel.getActivePlugins())
    {
        auto nodeId = plugin.nodeId;

        auto node = processingEngine.getNode(nodeId);

        if (node == nullptr || node->getProcessor() == nullptr)
        {
            DBG("Failed to retrieve processor for plugin: " + plugin.displayName);
            continue;
        }

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

std::map<juce::String, Status> AppController::restorePresetState(const juce::ValueTree& preset)
{
    std::map<juce::String, Status> results;

    if (!preset.hasType("NoTracPreset"))
        throw std::invalid_argument("Invalid preset format: missing 'NoTracPreset' root type.");

    auto chain = preset.getChildWithName("PluginChain");

    if (!chain.isValid())
        throw std::invalid_argument("Preset has no plugin chain.");

    clearPlugins();

    for (int i = 0; i < chain.getNumChildren(); ++i)
    {
        auto p = chain.getChild(i);

        const auto identifier = p["identifier"].toString();

        auto desc =  pluginRegistry
                                                    .getKnownPluginList()
                                                    .getTypeForIdentifierString(identifier);

        if (!desc)
        {
            results[identifier] = Status::failure("Plugin not found: " + identifier);
            continue;
        }

        ActivePlugin newPlugin;
        newPlugin.desc = *desc;
        newPlugin.displayName = p["name"].toString();
        newPlugin.bypassed = (bool) p["bypassed"];

        auto validation = canAddPlugin(*desc);
        if (validation.ok == false)
        {
            results[identifier] = Status::failure("Cannot add plugin: " + validation.error);
            continue;
        }

        // add the plugin to the processing engine to get a node ID
        auto result = addPlugin(*desc);

        // if adding the plugin failed, record the failure and continue to the next plugin
        if (!result.ok)
        {
            results[identifier] = Status::failure("Failed to add plugin: " + result.error);
            continue;
        }

        // if adding the plugin succeeded, restore its state
        results[identifier] = Status::success();
        auto nodeId = result.value;

        juce::MemoryBlock state;

        if (!state.fromBase64Encoding(p["stateBase64"].toString()))
            results[identifier] = Status::failure("Failed to decode plugin state for: " + identifier);
        
        else if (auto processor = processingEngine.getNode(nodeId)->getProcessor())
            processor->setStateInformation(state.getData(), (int)state.getSize());
    }

    return results;
}



std::unique_ptr<ActivePlugin> AppController::getPluginSnapshot(juce::AudioProcessorGraph::NodeID nodeId) const
{
    auto node = getNodeForId(nodeId);

    auto& plugin = pluginGraphModel.getActivePlugin(nodeId);

    if (node == nullptr || node->getProcessor() == nullptr)
            return nullptr;

    juce::MemoryBlock state;
    node->getProcessor()->getStateInformation(state);

    return std::make_unique<ActivePlugin>(
            ActivePlugin {
                plugin.desc,
                plugin.displayName,
                plugin.bypassed,
                plugin.chainIndex,
                plugin.nodeId,
                state
            });
}

Status AppController::removePlugin(juce::AudioProcessorGraph::NodeID nodeId)
{
    if (!pluginGraphModel.removePlugin(nodeId))
        return Status::failure("Failed to remove plugin from graph model.");

    processingEngine.removeNode(nodeId);
    return Status::success();
}

Status AppController::canAddPlugin(const juce::PluginDescription& desc) const
{
    if (desc.name.isEmpty())
        return Status::failure("Plugin description is invalid.");

    if (desc.numInputChannels <= 0)
        return Status::failure("Plugin " + desc.name + " has no audio input channels.");

    if (desc.numOutputChannels <= 0)
        return Status::failure("Plugin " + desc.name + " has no audio output channels.");

    return Status::success();
}

const std::vector<juce::AudioProcessorGraph::NodeID> AppController::getPluginOrder() const
{
    std::vector<juce::AudioProcessorGraph::NodeID> order;
    order.reserve(pluginGraphModel.getActivePlugins().size());

    for (const auto& plugin : pluginGraphModel.getActivePlugins())
        order.push_back(plugin.nodeId);

    return order;
}

juce::String AppController::getPluginDisplayName(juce::AudioProcessorGraph::NodeID nodeId) const
{
    return pluginGraphModel.getDisplayName(nodeId);
}

// void IOEngine::swapPlugin(size_t indexA, size_t indexB)
// {
//     if (indexA < chainNodes.size() && indexB < chainNodes.size())
//     {
//         std::swap(chainNodes[indexA], chainNodes[indexB]);
//         rebuildGraphConnections();
//     }
// }    


juce::AudioProcessorGraph::Node::Ptr AppController::getNodeForId(juce::AudioProcessorGraph::NodeID nodeId) const
{
    return processingEngine.getGraph().getNodeForId(nodeId);
}

void AppController::requestTogglePluginBypass(juce::AudioProcessorGraph::NodeID nodeId)
{
    const bool bypassed = pluginGraphModel.getActivePlugin(nodeId).bypassed;
    auto status = setPluginBypassed(nodeId, !bypassed);
    if (!status.ok)
        AppMessageBus::getInstance().error("Failed to bypass plugin", status.error);
}

Status AppController::setPluginBypassed(
    juce::AudioProcessorGraph::NodeID nodeId,
    bool shouldBypass)
{
    try
    {
        pluginGraphModel.setPluginBypassed(nodeId, shouldBypass);
        return Status::success();
    }
    catch (const std::exception& e)
    {
        return Status::failure(e.what());
    }
}

void AppController::requestSetPluginBypassed(
    juce::AudioProcessorGraph::NodeID nodeId,
    bool shouldBypass)
{
    auto status = setPluginBypassed(nodeId, shouldBypass);
    if (!status.ok)
        AppMessageBus::getInstance().error("Failed to bypass plugin", status.error);
}
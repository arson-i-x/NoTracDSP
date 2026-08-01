#pragma once

#include <functional>
#include <juce_core/juce_core.h>
#include "core/IOEngine.h"
#include "core/ProcessingEngine.h"
#include "core/PluginRegistry.h"
#include "core/PluginGraphModel.h"
#include "core/PresetManager.h"

#include "core/PluginWindow.h"

#include "Result.h"
#include "Status.h"

class AppController : public juce::ChangeBroadcaster,
                      public juce::ChangeListener,
                      private juce::AsyncUpdater
{
public:

    AppController();
    ~AppController() noexcept override; 

    void initializeIO();

    void clearPlugins();

    void shutdown();

    void setMasterGain(float /*gain*/) {}

    void addStatusListener(juce::ChangeListener *listener);
    void removeStatusListener(juce::ChangeListener *listener);

    // front end request method that creates and performs a command
    // Command message responses are sent to the AppMessageBus, which
    // the front end should set a callback for how it handles them.
    // AppMessageBus info messages are for general information, 
    // warning messages are for non-critical issues, and error 
    // messages are for critical issues that require user attention.
    // for example, if a command fails to perform, it should send an error message like undo or redo
    bool undo()
    {
        if (!undoManager.canUndo())
            return false;

        undoManager.undo();
        return true;
    }

    bool redo()
    {
        if (!undoManager.canRedo())
            return false;

        undoManager.redo();
        return true;
    }
    void requestAddPlugin(const juce::PluginDescription& desc);
    void requestRemovePlugin(juce::AudioProcessorGraph::NodeID nodeId);
    void requestOrderChange(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder);
    void requestSavePreset(const juce::String& presetName, const juce::ValueTree& presetState);
    void requestSetCurrentPreset(int presetId);
    void requestSetCurrentPreset(const juce::ValueTree& presetState);
    void requestSetPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId, bool bypassed);
    void requestTogglePluginBypass(juce::AudioProcessorGraph::NodeID nodeId);
    void togglePluginBypass(juce::AudioProcessorGraph::NodeID nodeId) { requestTogglePluginBypass(nodeId); }
    void requestBypassPlugin(juce::AudioProcessorGraph::NodeID nodeId) { requestTogglePluginBypass(nodeId); }
    void requestMidiMapPlugin(juce::AudioProcessorGraph::NodeID) {}

    Status setOrder(const std::vector<juce::AudioProcessorGraph::NodeID>& newOrder);
    Status setCurrentPreset(int presetId);
    Status setCurrentPreset(const juce::ValueTree& presetState);
    Status setPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId, bool shouldBypass);
    Status removePlugin(juce::AudioProcessorGraph::NodeID nodeId);
    Status canAddPlugin(const juce::PluginDescription& desc) const;
    Result<juce::AudioProcessorGraph::NodeID> addPlugin(const juce::PluginDescription& desc);
    Result<juce::AudioProcessorGraph::NodeID> addPlugin(const ActivePlugin& activePlugin);

    void setPresetManagerCallback(std::function<void()> callback)
    {
        presetManager.onPresetListUpdated = std::move(callback);
    }

    Status savePreset(const juce::String& presetName, const juce::ValueTree& presetState);

    // Returns a pointer to a PluginWindow which inherits from juce::Component for the given nodeId, or nullptr if the plugin window could not
    // be created. Append this PluginWindow to your component hierarchy to display the plugin's GUI. The PluginWindow will automatically handle resizing and closing events.
    // Note: The PluginWindow is a wrapper around the plugin's editor component. It is the caller's responsibility to manage the lifetime of the PluginWindow and ensure it is deleted when no longer needed.
    std::unique_ptr<PluginWindow> getPluginWindowForNodeId(juce::AudioProcessorGraph::NodeID nodeId);
    std::unique_ptr<PluginWindow> getPluginWindowForNode(juce::AudioProcessorGraph::NodeID nodeId) { return getPluginWindowForNodeId(nodeId); }

    juce::AudioProcessorGraph::Node::Ptr getNodeForId(juce::AudioProcessorGraph::NodeID nodeId) const;

    // used by backend commands to be able to undo plugin deletions and restorations, and to be able to restore plugin states from snapshots
    const std::vector<juce::AudioProcessorGraph::NodeID> getPluginOrder() const;
    std::unique_ptr<ActivePlugin> getPluginSnapshot(juce::AudioProcessorGraph::NodeID nodeId) const;
    juce::String getPluginDisplayName(juce::AudioProcessorGraph::NodeID nodeId) const;
    
    const PresetList<juce::String>& getPresets() const { return presetManager.getPresets(); };
    const juce::String& getCurrentPresetName() const { return presetManager.getCurrentPresetName(); };
    juce::AudioDeviceManager& getAudioDeviceManager() noexcept { return ioEngine.getAudioDeviceManager(); }
    const juce::AudioDeviceManager& getAudioDeviceManager() const noexcept { return ioEngine.getAudioDeviceManager(); }
    const DeviceStatus getDeviceStatus() const noexcept { return ioEngine.getDeviceStatus(); }
    
    // The plugin Graph Model is the authoritative source of truth for the plugin graph state, 
    // and the AppController provides access to it for other components that need to query or modify the plugin graph.
    // Listen to the pluginGraphModel for changes to the plugin graph. 
    // The AppController will handle the changes and update the ProcessingEngine accordingly,
    // as it also listens to the graph model for updates and uses it to direct authoritative truth into the actual processing pipeline.
    PluginGraphModel& getPluginGraphModel() noexcept { return pluginGraphModel; }
    const PluginGraphModel& getPluginGraphModel() const noexcept { return pluginGraphModel; }

    // The plugin registry is the authoritative source of truth for the known plugins,
    // and the AppController provides access to it for other components that need to query or modify the known plugins.
    // Query the plugin registry for known plugins and get a list of plugin descriptions.
    juce::KnownPluginList& getKnownPluginList() noexcept { return pluginRegistry.getKnownPluginList(); }
    const juce::KnownPluginList& getKnownPluginList() const noexcept { return pluginRegistry.getKnownPluginList(); }

    juce::ValueTree createPresetState() const;

private:
    juce::UndoManager undoManager;
    ProcessingEngine processingEngine;
    IOEngine ioEngine;
    PluginRegistry pluginRegistry;
    PluginGraphModel pluginGraphModel;
    PresetManager presetManager;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void handleAsyncUpdate() override;

    void rebuildGraphConnections();

    void connectStereo(
        juce::AudioProcessorGraph::NodeID source,
        juce::AudioProcessorGraph::NodeID dest);

    bool isPluginBypassed(juce::AudioProcessorGraph::NodeID nodeId) const;

    // tries to restore the state of the plugin graph from a snapshot, 
    // returns a Result containing the Status of each loaded plugin, 
    // and the overall Status of the operation
    std::map<juce::String, Status> restorePresetState(const juce::ValueTree& preset);
};
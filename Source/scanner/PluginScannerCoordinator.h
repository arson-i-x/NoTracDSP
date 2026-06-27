#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <iostream>
#include <filesystem>
#include <string>
#include "core/AppMessageBus.h"

class PluginScannerCoordinator : public juce::ChildProcessCoordinator
{
public:
    using ButtonStateFn = std::function<void(const juce::String& text, bool enabled)>;

    using DoneCallbackFn = std::function<void(const juce::KnownPluginList& newList)>;

    // Core plugin listing states
    juce::KnownPluginList knownPluginList;

    // Target tracking locations
    juce::File deadMansPedalFile;
    juce::File knownPluginsFile;
    juce::File workerScanXmlFile;

    // Define structural settings structures
    struct ScanSettings
    {
        juce::File directory;
        bool recursive = true;
        bool dontRescanIfAlreadyInList = true;
        bool allowAsyncInstantiation = false;
    };

    PluginScannerCoordinator();
    ~PluginScannerCoordinator() override = default;

    // Kicks off the asynchronous queue loop
    void startScan(
        ScanSettings& settings, 
        ButtonStateFn cb = nullptr,
        DoneCallbackFn doneCb = nullptr
    );

    const juce::KnownPluginList& getKnownPlugins() const;

private:
    // JUCE ChildProcessCoordinator overrides
    void handleMessageFromWorker(const juce::MemoryBlock& mb) override;
    void handleConnectionLost() override;
    // Processing management steps
    void scanNextItemInQueue();
    bool launchWorker();
    bool sendSettingsToWorker();
    bool workerIsStillRunning() const;
    void markPluginAsFailed(const juce::String& pluginPath);
    void setUi(const juce::String& text, bool enabled);
    void finish();
    void scheduleNextScan();
    void saveKnownPluginList();

    // Constant tracking identifiers
    static constexpr const char* kIPC = "NoTracDSPVST3Scan";

    // Asynchronous file tracking loop variables
    juce::Array<juce::File> filesToScan;
    size_t currentFileIndex = 0;
    juce::File currentFile;
    ScanSettings pendingSettings;

    bool workerReportedDone = false;

    // UI state preservation functions
    // Callback types for updating UI component layers
    ButtonStateFn onButtonState;
    DoneCallbackFn onDone;

    juce::String currentlyScanningPlugin;
    bool scanTimedOut = false;
    bool knownPluginListDirty = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginScannerCoordinator)
};

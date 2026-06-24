#include "PluginScannerCoordinator.h"

static juce::MemoryBlock toBlock(const juce::String& s) { return { s.toRawUTF8(), (size_t) s.getNumBytesAsUTF8() }; }

PluginScannerCoordinator::PluginScannerCoordinator(AppMessageBus& messageBus) : messageBus(messageBus)
{
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("NoTracDSP");
    appData.createDirectory();
    
    deadMansPedalFile = appData.getChildFile("scan_pedal.txt");
    knownPluginsFile = appData.getChildFile("known_plugins.xml");
    
    if (knownPluginsFile.existsAsFile())
        if (auto xml = juce::XmlDocument::parse(knownPluginsFile))
            knownPluginList.recreateFromXml(*xml);
}

void PluginScannerCoordinator::saveKnownPluginList() 
{
    if (auto xml = knownPluginList.createXml())
        xml->writeTo(knownPluginsFile);
}

// 1. THIS KICKS OFF THE CHAIN WITHOUT BLOCKING
void PluginScannerCoordinator::startScan(ScanSettings& settings, ButtonStateFn cb) 
{
    currentFileIndex = 0;
    onButtonState = std::move(cb);
    pendingSettings = settings;

    if (!settings.directory.isDirectory())
    {
        finish();
        return;
    }

    setUi("Finding VST3 files...", false);

    juce::Timer::callAfterDelay(50, [this]
    {
        filesToScan = pendingSettings.directory.findChildFiles(
            juce::File::findFiles,
            pendingSettings.recursive,
            "*.vst3");

        DBG("Found " + juce::String(filesToScan.size()) + " VST3 files.");

        scheduleNextScan();
    });
}

// 2. THE QUEUE WORKER
void PluginScannerCoordinator::scanNextItemInQueue()
{
    workerReportedDone = false;
    scanTimedOut = false;

    if (currentFileIndex >= filesToScan.size())
    {
        finish();
        return;
    }

    currentFile = filesToScan[currentFileIndex];

    setUi("Scanning (" 
          + juce::String(currentFileIndex + 1)
          + "/"
          + juce::String(filesToScan.size())
          + ") "
          + currentFile.getFileName(),
          false);

    if (!launchWorker() || !sendSettingsToWorker())
    {
        currentFileIndex++;
        scheduleNextScan();
    }
}

void PluginScannerCoordinator::markPluginAsFailed(const juce::String& pluginPath)
{
    juce::String pluginName = juce::File(pluginPath).getFileName();
    // do nothing now, but we could add failed plugins to a list and save it to disk if we want to track them
    //DBG("PluginScanner/Coordinator [DEBUG] Marking plugin as failed: " + pluginName);    
}


bool PluginScannerCoordinator::launchWorker() 
{
    currentlyScanningPlugin = currentFile.getFullPathName();

    scanTimedOut = false;

    juce::Timer::callAfterDelay(15000, [this, pluginPath = currentlyScanningPlugin]
    {
        if (currentlyScanningPlugin == pluginPath && !workerReportedDone)
        {
            scanTimedOut = true;

            DBG("Plugin scan timed out: " + juce::File(pluginPath).getFileName());

            killWorkerProcess();
            markPluginAsFailed(pluginPath);

            workerReportedDone = true;
            currentFileIndex++;

            scheduleNextScan();
        }
    });
    //DBG("PluginScanner/Coordinator [DEBUG] Launching worker process for: " + currentFile.getFullPathName());
    //DBG("===================================================================================");
    return launchWorkerProcess(juce::File(SCANNER_WORKER_PATH), kIPC, 2000);
}
bool PluginScannerCoordinator::sendSettingsToWorker() 
{
    //DBG("PluginScanner/Coordinator [DEBUG] Sending settings to worker for: " + currentFile.getFullPathName());

    juce::ValueTree v("scanSettings");
    // CRITICAL: We pass the exact solitary file, NOT the whole folder!
    v.setProperty("directory", currentFile.getFullPathName(), nullptr); 
    v.setProperty("recursive", pendingSettings.recursive, nullptr);
    v.setProperty("dontRescan", pendingSettings.dontRescanIfAlreadyInList, nullptr);
    v.setProperty("allowAsync", pendingSettings.allowAsyncInstantiation, nullptr);
    v.setProperty("deadMansPedal", deadMansPedalFile.getFullPathName(), nullptr);

    std::unique_ptr<juce::XmlElement> xml(v.createXml());
    //DBG("Created XML for worker: " + xml->toString());
    return xml != nullptr && sendMessageToWorker(toBlock(xml->toString()));
}

// 3. SUCCESS CALLBACK
void PluginScannerCoordinator::handleMessageFromWorker(const juce::MemoryBlock& mb) 
{
    const auto msg = juce::String::fromUTF8((const char*) mb.getData(), (int) mb.getSize()).trim();
    
    // Check if the worker sent back actual plugin data XML instead of just "done"
    if (msg.startsWith ("[WORKER_LOG]"))
    {
        // Strip the tag and print it to your Frontend IDE console!
        //DBG (msg.substring (12)); 
        return; // Return early so we don't advance the queue loop yet!
    }

    //DBG("PluginScanner/Coordinator [DEBUG] Received response from worker.");

    if (msg.startsWith("[WORKER_OUTPUT]")) 
    {
        const auto output = msg.substring(15);
        //DBG("PluginScanner/Coordinator [DEBUG] Received plugin data from worker, saving to local known plugin list.");
        if (auto xml = juce::XmlDocument::parse(output))
        {
            juce::KnownPluginList tempList;
            tempList.recreateFromXml(*xml);

            for (const auto& desc : tempList.getTypes())
                knownPluginList.addType(desc);

            knownPluginListDirty = true;
        }
        //DBG("PLUGIN COUNT = "
        // + juce::String(knownPluginList.getTypes().size()));
    }

    if (msg.startsWith("[WORKER_SCANNING]"))
    {
        currentlyScanningPlugin = msg.substring(17).trim();
        //DBG("Worker started scanning: " + currentlyScanningPlugin);
        return;
    }

    if (msg == "[WORKER_DONE]")
    {
        if (scanTimedOut)
            return;

        workerReportedDone = true;
        currentFileIndex++;

        scheduleNextScan();
        return;
    }
}

void PluginScannerCoordinator::scheduleNextScan()
{
    juce::Timer::callAfterDelay(25, [this]
    {
        scanNextItemInQueue();
    });
}

juce::String getFileName(const juce::String& path) {
    // Scan backwards for either a forward or backward slash
    size_t lastSlash = path.lastIndexOfAnyOf("/\\");
    
    // If no slash is found, the path itself is just the file name
    if (lastSlash == std::string::npos) {
        return path;
    }
    
    // Extract everything after the last slash position
    return juce::String(path.substring(lastSlash + 1));
}

// 4. CRASH CALLBACK (No more unknown crashes!)
void PluginScannerCoordinator::handleConnectionLost() 
{
    // //DBG("====================");
    // //DBG("CURRENT INDEX: " + juce::String(currentFileIndex));
    // //DBG("CURRENT FILE: " + currentFile.getFileName());
    // //DBG("WORKER REPORTED DONE: " + juce::String(int(workerReportedDone)));
    // //DBG("====================");
    if (workerReportedDone)
    {
        return;
    }
    // // Because currentFile is tracked safely in our frontend class state, 
    // // we know EXACTLY what plugin just crashed without any disk file checks.
    // //DBG("PluginScanner/Coordinator [DEBUG] Plugin crashed during scan: " + currentFile.getFullPathName());
    // juce::String crashed = getFileName(currentFile.getFullPathName());
    
    // juce::MessageManager::callAsync([this, crashed] {
    //     juce::AlertWindow::showMessageBoxAsync(
    //         juce::AlertWindow::WarningIcon, "Plugin Scanner", 
    //         "Plugin crashed during scan:\n" + crashed, "Continue", nullptr, 
    //         juce::ModalCallbackFunction::create([this](int) {
    //             // When the user clicks "Continue", advance to the next file automatically
    //             currentFileIndex++;
    //             scanNextItemInQueue();
    //         }));
    // });
    DBG("PluginScanner/Coordinator [DEBUG] Worker Connection lost and not reported done. \
        Marking plugin as failed: " + currentFile.getFullPathName());
}

void PluginScannerCoordinator::setUi(const juce::String& text, bool enabled) 
{
    if (!onButtonState) return;
    auto cb = onButtonState;
    juce::MessageManager::callAsync([cb, text, enabled] { cb(text, enabled); });
}

void PluginScannerCoordinator::finish() 
{
    killWorkerProcess();

    if (knownPluginListDirty)
    {
        saveKnownPluginList();
        knownPluginListDirty = false;
    }

    setUi("Scan Plugins", true);
}
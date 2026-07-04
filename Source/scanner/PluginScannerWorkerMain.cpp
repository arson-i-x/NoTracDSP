#include "PluginScannerWorkerMain.h"

static juce::MemoryBlock toBlock(const juce::String& s) { return { s.toRawUTF8(), (size_t) s.getNumBytesAsUTF8() }; }

void VST3WorkerApplication::VST3ScanWorker::handleMessageFromCoordinator (const juce::MemoryBlock& mb) 
{
        const auto xmlText = juce::String::fromUTF8 ((const char*) mb.getData(), (int) mb.getSize());
        auto xml = juce::XmlDocument::parse (xmlText);
        
        if (xml == nullptr || xml->hasTagName ("scanSettings") == false) 
            return;

        juce::ValueTree settings = juce::ValueTree::fromXml (*xml);
        
        // Deep copy variables safely into the lambda by value (NO raw memory references)
        const juce::File targetFile (settings.getProperty ("directory").toString());
        const bool dontRescan = (bool) settings.getProperty ("dontRescan");
        const bool allowAsync = (bool) settings.getProperty ("allowAsync");
        const juce::File pedal (settings.getProperty ("deadMansPedal").toString());

        // Jump instantly to the main thread context to handle WarpPal/Direct2D frameworks safely
        juce::MessageManager::callAsync ([this, targetFile, dontRescan, allowAsync, pedal] 
        {
            // Objects live safely on this specific lambda invocation scope context
            juce::KnownPluginList localList;
            
            // Explicitly point the scanner directly at the solitary file path target
            juce::FileSearchPath singleFilePath;
            singleFilePath.add (targetFile);
            
            sendMessageToCoordinator(toBlock("[WORKER_SCANNING]" + targetFile.getFullPathName()));

            for (auto* f : fm.getFormats()) 
            {
                juce::OwnedArray<juce::PluginDescription> descriptions;
                    f->findAllTypesForFile(
                        descriptions,
                        targetFile.getFullPathName());

                for (auto* desc : descriptions)
                {
                    localList.addType(*desc);
                }
            }

            const auto localListXml = localList.createXml()->toString();

            // send the result as xml to the coordinator
            const juce::String xmlString = "[WORKER_OUTPUT] " + localListXml;
            sendMessageToCoordinator (toBlock(xmlString));

            // Notify the coordinator over the pipe channel that the file is fully processed
            const juce::String done ("[WORKER_DONE]");
            sendMessageToCoordinator (toBlock(done));
            
            juce::JUCEApplication::getInstance()->quit();

        });
    }

void WorkerToFrontendLogger::logMessage (const juce::String& message)
{
    // Wrap the log message in a simple XML or text tag so the frontend can identify it
    juce::String logPayload = "[WORKER_LOG] " + message;
    
    juce::MemoryBlock mb;
    mb.append (logPayload.toRawUTF8(), logPayload.getNumBytesAsUTF8());
        
    worker.sendMessageToCoordinator (mb);
}
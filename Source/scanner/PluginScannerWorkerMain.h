#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>

class WorkerToFrontendLogger : public juce::Logger
{
public:
    WorkerToFrontendLogger (juce::ChildProcessWorker& w) : worker (w) {}

protected:
    void logMessage (const juce::String& message) override;

private:
    juce::ChildProcessWorker& worker;
};

class VST3WorkerApplication final : public juce::JUCEApplication
{
public:
    VST3WorkerApplication() = default;

    const juce::String getApplicationName() override       { return "VST3ScanWorker"; }
    const juce::String getApplicationVersion() override    { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise (const juce::String& commandLineArguments) override
    {
        worker = std::make_unique<VST3ScanWorker>();

        // Under START_JUCE_APPLICATION, the event loops are fully live right here.
        // JUCE injects the raw command line arguments cleanly into this string.
        if (! worker->initialiseFromCommandLine (commandLineArguments, VST3ScanWorker::kIPC, 10000))
        {
            // If the coordinator isn't responding or arguments are malformed, exit safely.
            quit();
            return;
        }

        // --- INSIDE YOUR WORKER'S MAIN() OR INITIALIZATION ---
        // Right after initializing the worker:
        customLogger.reset(new WorkerToFrontendLogger(*worker));
        juce::Logger::setCurrentLogger (customLogger.get());
        #if JUCE_DEBUG
            #undef DBG
            #define DBG(text) juce::Logger::writeToLog(text)
        #endif
        //DBG("Custom logger initialized.");
    }

    void shutdown() override
    {
        juce::Logger::setCurrentLogger (nullptr);
        customLogger.reset();
        worker.reset();
    }

    void systemRequestedQuit() override                  { quit(); }
    void anotherInstanceStarted (const juce::String&) override {}

private:
    class VST3ScanWorker final : public juce::ChildProcessWorker 
    {
    public:
        static constexpr const char* kIPC = "NoTracDSPVST3Scan";

        VST3ScanWorker() 
        {
            juce::addDefaultFormatsToManager(fm);
        }

        ~VST3ScanWorker() override = default;

        void handleMessageFromCoordinator (const juce::MemoryBlock& mb) override;
    
        void handleConnectionLost() override 
        {
            juce::JUCEApplication::getInstance()->quit();
        }

    private:
        juce::AudioPluginFormatManager fm;
        std::unique_ptr<juce::FileLogger> fileLogger;
    };

    std::unique_ptr<VST3ScanWorker> worker;
    std::unique_ptr<WorkerToFrontendLogger> customLogger;
};

// ==============================================================================
// 3. THE OFFICIAL MACRO ENTRY POINT
// ==============================================================================
// This expands natively into a safe WinMain/main wrapper depending on your OS target.
START_JUCE_APPLICATION (VST3WorkerApplication)

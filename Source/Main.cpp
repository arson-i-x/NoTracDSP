#include "MainComponent.h"

class QuadCorePrototypeApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override       { return "NoTrac DSP Prototype"; }
    const juce::String getApplicationVersion() override    { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise (const juce::String& commandLine) override
    {
        juce::ignoreUnused (commandLine);
        // Create a file logger so `Logger::writeToLog` output is captured to disk
        juce::File logFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                     .getChildFile("NoTracDSP")
                     .getChildFile("NoTracDSP" + juce::Time::getCurrentTime().toString (true, true) + ".log");

        // 2. Create the file logger instance
        // Parameters: Target file, Welcome message, Max line length (0 for unlimited)
        fileLogger = std::make_unique<juce::FileLogger>(logFile, "--- Log Started ---", 0);

        // 3. Register it globally. JUCE does not take ownership of this pointer.
        juce::Logger::setCurrentLogger(fileLogger.get());

        mainWindow.reset (new MainWindow (getApplicationName()));
    }

    void shutdown() override
    {
        juce::Logger::setCurrentLogger (nullptr);
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    class MainWindow final : public juce::DocumentWindow {
        public:
            explicit MainWindow (juce::String name)
                : juce::DocumentWindow (name,
                                        juce::Desktop::getInstance().getDefaultLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId),
                                        allButtons)
            {
                setUsingNativeTitleBar (false);

                // Add the main content component to the window, and make it resize with window resizing.
                setContentOwned (new MainComponent(), true);

            #if JUCE_IOS || JUCE_ANDROID
                setFullScreen (true);
            #else
                setResizable (true, true);
                centreWithSize (getWidth(), getHeight());
            #endif
                // set the window to be visible, so the user can interact with it.
                setVisible (true);
            }

            void closeButtonPressed() override
            {
                juce::JUCEApplication::getInstance()->systemRequestedQuit();
            }
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<juce::FileLogger> fileLogger;
};

START_JUCE_APPLICATION (QuadCorePrototypeApplication)
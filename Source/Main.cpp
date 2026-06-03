#include "MainComponent.h"

class QuadCorePrototypeApplication final : public juce::JUCEApplication
{
public:
    // const juce::String JUCE_APPLICATION_NAME_STRING = "QuadCore Prototype";
    // const juce::String JUCE_APPLICATION_VERSION_STRING = "0.1.0";

    // Use these two methods to retrieve the name and version of your application from CMakeLists.txt
    const juce::String getApplicationName() override       { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override    { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise (const juce::String& commandLine) override
    {
        juce::ignoreUnused (commandLine);
        mainWindow.reset (new MainWindow (getApplicationName()));
    }

    void shutdown() override
    {
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
};

START_JUCE_APPLICATION (QuadCorePrototypeApplication)
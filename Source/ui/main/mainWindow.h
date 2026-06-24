    #include <juce_gui_basics/juce_gui_basics.h>
    #include "ui/main/MainComponent.h"

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
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
class PluginWindow : public juce::DocumentWindow
{
public:
    PluginWindow(juce::AudioProcessor* processor)
        : DocumentWindow(processor->getName(),
                         juce::Colours::black,
                         DocumentWindow::closeButton)
    {
        // if processor is null, the application will crash
        jassert(processor != nullptr);


        if (!processor->hasEditor())
        {
            setContentOwned(new juce::Label({}, "This plugin has no editor."), true);
            centreWithSize(300, 120);
            setVisible(true);
            return;
        }

        auto* editor = processor->createEditorAndMakeActive();

        if (editor == nullptr)
        {
            setContentOwned(new juce::Label({}, "Failed to create plugin editor."), true);
            centreWithSize(300, 120);
            setVisible(true);
            return;
        }

        setContentOwned(editor, true);
        centreWithSize(editor->getWidth(), editor->getHeight());
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

    void resized() override
    {
        DocumentWindow::resized();
    }

    ~PluginWindow()
    {
        setVisible(false);
    }
};
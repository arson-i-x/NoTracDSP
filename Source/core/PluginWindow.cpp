#include "PluginWindow.h"

void PluginWindow::closeButtonPressed()
{
    setVisible(false);
}

PluginWindow::PluginWindow(juce::AudioProcessor& processor)
    : processor(processor), DocumentWindow(processor.getName(),
                            juce::Colours::black,
                            DocumentWindow::closeButton)
{
    JUCE_ASSERT_MESSAGE_MANAGER_IS_LOCKED;

    if (!processor.hasEditor())
    {
        setContentOwned(new juce::Label({}, "This plugin has no editor."), true);
        centreWithSize(300, 120);
        setVisible(true);
        return;
    }

    processor.createEditorAndMakeActive();

    auto* editor = processor.getActiveEditor();

    if (editor == nullptr)
    {
        AppMessageBus::getInstance().warning("Plugin Editor Error", "The plugin editor could not be created.");
        setContentOwned(new juce::Label({}, "This plugin has no editor."), true);
        centreWithSize(300, 120);
        setVisible(true);
        return;
    }

    setContentOwned(editor, true);
    centreWithSize(editor->getWidth(), editor->getHeight());
    setVisible(true);
}
#include "DirectoryManager.h"

DirectoryManager::DirectoryManager(const juce::String &message)
{
    fileChooser = std::make_unique<juce::FileChooser>(
        message,
        juce::File::getSpecialLocation(juce::File::userHomeDirectory));

    chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;
}

DirectoryManager::~DirectoryManager()
{
    fileChooser.reset();
    chooserFlags = 0;
}

void DirectoryManager::chooseDirectory()
{
    if (fileChooser)
    {
        fileChooser->launchAsync(
            chooserFlags,
            [this](const juce::FileChooser &chooser)
            {
                // This block executes ONLY when the user closes the window
                auto resultFile = chooser.getResult();

                selectedDirectory = resultFile;

                searchPaths.push_back(selectedDirectory);

                if (onDirectoryChosen)
                    onDirectoryChosen(selectedDirectory);
            });
    }
}

std::vector<juce::File> DirectoryManager::getSearchPaths() const
{
    return searchPaths;
}
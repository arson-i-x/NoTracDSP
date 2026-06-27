#include <juce_gui_basics/juce_gui_basics.h>

class DirectoryManager
{
public:
    DirectoryManager(const juce::String &message = "Select a Directory");
    ~DirectoryManager();
    std::function<void(const juce::File&)> onDirectoryChosen;
    void addSearchPath(const juce::File &path);
    std::vector<juce::File> getSearchPaths() const;
    void chooseDirectory();
private:
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File selectedDirectory;
    std::vector<juce::File> searchPaths;
    int chooserFlags;
};
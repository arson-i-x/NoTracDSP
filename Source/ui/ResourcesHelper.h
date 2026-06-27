#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>

enum class IconType
{
    Undo,
    Redo,
    Remove,
    Bypass,
    Settings
};

class ImageResources
{
public:
    static juce::Image getIcon(IconType type)
    {
        switch (type)
        {
            case IconType::Undo:
                return juce::ImageCache::getFromMemory(
                    BinaryData::undo_png,
                    BinaryData::undo_pngSize);

            case IconType::Redo:
                return juce::ImageCache::getFromMemory(
                    BinaryData::redo_png,
                    BinaryData::redo_pngSize);

            // case IconType::Remove:
            //     return juce::ImageCache::getFromMemory(
            //         BinaryData::remove_png,
            //         BinaryData::remove_pngSize);

            case IconType::Bypass:
                return juce::ImageCache::getFromMemory(
                    BinaryData::bypass_png,
                    BinaryData::bypass_pngSize);

            case IconType::Settings:
                return juce::ImageCache::getFromMemory(
                    BinaryData::settings_png,
                    BinaryData::settings_pngSize);
        }

        return {};
    }
};
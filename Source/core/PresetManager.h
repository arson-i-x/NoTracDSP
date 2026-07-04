#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "core/AudioEngine.h"

template <typename T>
class PresetList 
{
public:
    PresetList() = default;

    // Construct from a juce::Array
    PresetList(const juce::Array<T>& arr) 
    {
        data.reserve(arr.size());
        for (const auto& item : arr) {
            data.push_back(item);
        }
    }

    // Construct with a specific size
    explicit PresetList(size_t size) : data(size) {}

    // 1-Indexed Non-const Access Operator
    T& operator[](size_t index) 
    {
        // Enforce the 1-indexed precondition in debug builds
        jassert(index >= 1 && index <= data.size()); 
        return data[index - 1]; 
    }

    // 1-Indexed Const Access Operator (Required for read-only situations)
    const T& operator[](size_t index) const 
    {
        jassert(index >= 1 && index <= data.size()); 
        return data[index - 1]; 
    }

    // Pass-through helpers to make UI work easy
    size_t size() const noexcept { return data.size(); }
    void clear()               { data.clear(); }
    void push_back(const T& val) { data.push_back(val); }

private:
    std::vector<T> data;
};

class PresetManager : public juce::ChangeBroadcaster
{
private:
    juce::File presetDirectory;

    bool stateModified = false;

    PresetList<juce::String> presets;

    juce::String currentPresetName = "init";

    std::optional<juce::UndoManager> undoManager;
public:
    PresetManager();
    ~PresetManager();
    void loadPresets();
    Status savePreset(const juce::String& fileName, const juce::ValueTree& saveState);
    Status deletePreset(const juce::File& preset);
    Status setCurrentPreset(int selectedIndex);
    Status setCurrentPreset(juce::ValueTree presetState);
    const juce::String getCurrentPresetName() const { return currentPresetName; }
    PresetList<juce::String> getPresets() const;
    bool isModified() const { return stateModified; }
    juce::ValueTree createPresetState(const juce::String& presetName) const;
    std::function<void(juce::ValueTree)> onPresetChanged; // Callback for when presets are changed

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
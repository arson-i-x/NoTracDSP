#pragma once

#include <optional>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include "core/PluginGraphModel.h"

class AppController;

struct PluginGraphItem
{
    juce::AudioProcessorGraph::NodeID nodeId { 0 };
    juce::String displayName;
    bool bypassed = false;
    juce::Rectangle<int> bounds;
    juce::Rectangle<int> removeButtonBounds;
};

class PluginGraphViewComponent : public juce::Component,
                                 public juce::ChangeListener
{
public:
    PluginGraphViewComponent(AppController& app);
    ~PluginGraphViewComponent() override;

    std::function<void(juce::AudioProcessorGraph::NodeID)> onPluginDoubleClicked;
    std::function<void(const std::vector<juce::AudioProcessorGraph::NodeID>&)> onOrderChanged;
    std::function<void(juce::AudioProcessorGraph::NodeID)> onPluginBypassed;
    std::function<void(juce::AudioProcessorGraph::NodeID)> onPluginRemoved;
    std::function<void(juce::AudioProcessorGraph::NodeID)> onMidiMapRequested;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent& event) override;
    int getInsertIndexForX(int x) const;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    PluginGraphModel& model;
    
    std::vector<PluginGraphItem> items;

    std::optional<juce::AudioProcessorGraph::NodeID> selectedPluginId;
    int draggingIndex = -1;
    int originalDragIndex = -1;
    int hoveredIndex = -1;
    juce::Point<int> dragOffset;
    bool didDrag = false;

    void refreshPluginItems();
    void getItemsFromModel();
    int getItemIndexAt(juce::Point<int> position) const;
    void layoutItems();
    void layoutItemsExceptDragging();
    void sendOrderChanged();
};
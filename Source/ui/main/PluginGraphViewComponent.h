#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>

struct PluginGraphItem
{
    juce::AudioProcessorGraph::NodeID nodeId;
    juce::String name;
    juce::Rectangle<int> bounds;
    juce::Rectangle<int> removeButtonBounds;
    bool bypassed = false;
};

class PluginGraphViewComponent : public juce::Component
{
public:
    PluginGraphViewComponent();

    std::function<void(juce::AudioProcessorGraph::NodeID)> onPluginDoubleClicked;
    std::function<void(const std::vector<juce::AudioProcessorGraph::NodeID>&)> onOrderChanged;
    std::function<void(juce::AudioProcessorGraph::NodeID)> onPluginBypassed;
    std::function<void(juce::AudioProcessorGraph::NodeID)> onPluginRemoved;
    std::function<void(juce::AudioProcessorGraph::NodeID)> onMidiMapRequested;

    void setPlugins(const std::vector<PluginGraphItem>& newItems);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent& event) override;
    int getInsertIndexForX(int x) const;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    juce::AudioProcessorGraph::NodeID* getSelectedPluginId() const;


private:
    std::vector<PluginGraphItem> items;

    std::unique_ptr<juce::AudioProcessorGraph::NodeID> selectedPluginId;
    int draggingIndex = -1;
    int originalDragIndex = -1;
    int hoveredIndex = -1;
    juce::Point<int> dragOffset;
    bool didDrag = false;

    int getItemIndexAt(juce::Point<int> position) const
    {
        for (int i = 0; i < (int) items.size(); ++i)
            if (items[i].bounds.contains(position))
                return i;

        return -1;
    }

    void layoutItems()
    {
        int x = 40;
        const int y = 70;
        const int width = 150;
        const int height = 64;
        const int gap = 50;

        for (auto& item : items)
        {
            item.bounds = { x, y, width, height };
            x += width + gap;
        }
    }

    void layoutItemsExceptDragging()
    {
        int x = 40;
        const int y = 70;
        const int width = 150;
        const int height = 64;
        const int gap = 50;

        for (int i = 0; i < (int) items.size(); ++i)
        {
            if (i != draggingIndex)
                items[i].bounds = { x, y, width, height };

            x += width + gap;
        }
    }

    void sendOrderChanged()
    {
        if (!onOrderChanged)
            return;
        DBG("NEW UI ORDER:");
        for (const auto& item : items)
            DBG(item.name);
        std::vector<juce::AudioProcessorGraph::NodeID> newOrder;

        for (const auto& item : items)
            newOrder.push_back(item.nodeId);

        onOrderChanged(newOrder);
    }
};
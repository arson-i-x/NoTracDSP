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
    void setPlugins(const std::vector<PluginGraphItem>& newItems)
    {
        items = newItems;
        layoutItems();
        repaint();
    }

    std::function<void(juce::AudioProcessorGraph::NodeID)> onPluginDoubleClicked;

    // Tell MainComponent / AudioEngine that visual order changed
    std::function<void(const std::vector<juce::AudioProcessorGraph::NodeID>&)> onOrderChanged;

    std::function<void(juce::AudioProcessorGraph::NodeID)> onPluginBypassed;

    std::function<void(juce::AudioProcessorGraph::NodeID)> onPluginRemoved;

    std::function<void(juce::AudioProcessorGraph::NodeID)> onMidiMapRequested;

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff101418));

        // Draw cables first
        for (int i = 0; i + 1 < (int) items.size(); ++i)
        {
            const auto& a = items[i];
            const auto& b = items[i + 1];

            g.setColour(juce::Colour(0xff8bd3ff));

            g.drawLine((float) a.bounds.getRight(),
                       (float) a.bounds.getCentreY(),
                       (float) b.bounds.getX(),
                       (float) b.bounds.getCentreY(),
                       2.0f);
        }

        // Draw plugin boxes
        for (int i = 0; i < (int) items.size(); ++i)
        {
            const auto& item = items[i];
            const bool isSelected = selectedPluginId && item.nodeId == *selectedPluginId;
            const bool isBypassed = item.bypassed;
            if (isSelected)
                g.setColour(juce::Colour(0xff405a78));
            else if (isBypassed) {
                DBG("Plugin " + item.name + " is bypassed. Painting with bypassed color.");
                g.setColour(juce::Colour(0x80ff0000));
            }
            else
                g.setColour(juce::Colour(0xff2b3440));

            const bool isDragging = i == draggingIndex;
            g.fillRoundedRectangle(item.bounds.toFloat(), 8.0f);

            if (isSelected)
            {
                if (isBypassed)
                    g.setColour(juce::Colour(0xffff0000));
                else
                    g.setColour(juce::Colour(0xffffd700));
                g.drawRoundedRectangle(item.bounds.toFloat(), 8.0f, 3.0f);
            }

            if (isDragging)
            {
                g.setColour(juce::Colour(0x55ffffff));
                g.drawRoundedRectangle(item.bounds.toFloat().expanded(3.0f), 10.0f, 2.0f);
            }

            const bool isHovered = i == hoveredIndex;

            if (isHovered)
            {
                auto removeArea = item.bounds.reduced(8).removeFromTop(20).removeFromRight(20);

                const_cast<PluginGraphItem&>(item).removeButtonBounds = removeArea;

                g.setColour(juce::Colour(0xffaa3333));
                g.fillEllipse(removeArea.reduced(3).toFloat());

                g.setColour(juce::Colours::white);
                g.drawText(juce::String("x"), removeArea, juce::Justification::centred);
            }

            g.setColour(juce::Colour(0xffdbe4ee));
            g.drawText(item.name,
                       item.bounds.reduced(8),
                       juce::Justification::centred);
        }
    }

    void resized() override
    {
        layoutItems();
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        const auto pos = event.position.toInt();

        draggingIndex = getItemIndexAt(pos);
        originalDragIndex = draggingIndex;
        didDrag = false;

        // remove button click
        for (const auto& item : items)
        {
            if (item.removeButtonBounds.contains(pos))
            {
                if (onPluginRemoved)
                DBG("Removing plugin " + item.name);
                    onPluginRemoved(item.nodeId);

                return;
            }
        }

        if (draggingIndex >= 0)
        {
            selectedPluginId = std::make_unique<juce::AudioProcessorGraph::NodeID>(
                items[draggingIndex].nodeId);

            dragOffset = pos - items[draggingIndex].bounds.getPosition();
        }
        else
        {
            selectedPluginId.reset();
        }

        repaint();
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (draggingIndex < 0 || !event.mods.isLeftButtonDown())
            return;

        didDrag = true;

        auto newPos = event.position.toInt() - dragOffset;

        items[draggingIndex].bounds.setPosition(
            newPos.x,
            items[draggingIndex].bounds.getY());

        repaint();
    }

    void mouseMove(const juce::MouseEvent& event) override
    {
        const auto pos = event.position.toInt();
        hoveredIndex = getItemIndexAt(pos);
        repaint();
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        hoveredIndex = -1;
        repaint();
    }

    void mouseUp(const juce::MouseEvent& event) override
    {
        if (draggingIndex < 0)
            return;

        if (event.mods.isRightButtonDown())
        {
            if (selectedPluginId && onPluginBypassed)
                onPluginBypassed(*selectedPluginId);

            draggingIndex = -1;
            originalDragIndex = -1;
            layoutItems();
            repaint();
            return;
        }

        if (selectedPluginId != nullptr && event.mods.isMiddleButtonDown())
        {
            if (onMidiMapRequested)
                onMidiMapRequested(*selectedPluginId);

            draggingIndex = -1;
            layoutItems();
            repaint();
            return;
        }


        if (didDrag)
        {
            const auto draggedItem = items[draggingIndex];
            const int dropX = draggedItem.bounds.getCentreX();

            items.erase(items.begin() + draggingIndex);

            int insertIndex = 0;

            for (const auto& item : items)
            {
                if (dropX > item.bounds.getCentreX())
                    ++insertIndex;
            }

            insertIndex = juce::jlimit(0, (int) items.size(), insertIndex);

            items.insert(items.begin() + insertIndex, draggedItem);

            draggingIndex = -1;
            originalDragIndex = -1;
            didDrag = false;

            layoutItems();
            sendOrderChanged();
            repaint();
            return;
        }

        draggingIndex = -1;
        originalDragIndex = -1;
        repaint();
    }

    int getInsertIndexForX(int x) const
    {
        int insertIndex = 0;

        for (int i = 0; i < (int) items.size(); ++i)
        {
            if (i == draggingIndex)
                continue;

            if (x > items[i].bounds.getCentreX())
                ++insertIndex;
        }

        return insertIndex;
    }

    void mouseDoubleClick(const juce::MouseEvent& event) override
    {
        const int index = getItemIndexAt(event.position.toInt());

        if (index >= 0 && onPluginDoubleClicked)
            onPluginDoubleClicked(items[index].nodeId);
    }

    juce::AudioProcessorGraph::NodeID* getSelectedPluginId() const
    {
        return selectedPluginId.get();
    }

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
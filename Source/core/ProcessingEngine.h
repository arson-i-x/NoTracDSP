#pragma once

#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>

class ProcessingEngine
{
public:
    ProcessingEngine() {
        inputNode = graph.addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));

        outputNode = graph.addNode(
            std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
                juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
    };

    ~ProcessingEngine() {
        graph.removeNode(inputNode->nodeID);
        graph.removeNode(outputNode->nodeID);
        graph.releaseResources();
    }

    void prepare(double sampleRate, int blockSize, int numInputChannels, int numOutputChannels)
    {
        inputChannels = numInputChannels;
        outputChannels = numOutputChannels;

        graph.setPlayConfigDetails(
            inputChannels,
            outputChannels,
            sampleRate,
            blockSize
        );

        graph.prepareToPlay(sampleRate, blockSize);
    }

    void process(
        juce::AudioBuffer<float>& buffer,
        juce::MidiBuffer& midi)
    {
        graph.processBlock(buffer, midi);
    }

    void release()
    {
        graph.releaseResources();
    }

    void clearGraph()
    {
        graph.clear();
        connections.clear();
    }

    void clearConnections() 
    {
        for (const auto& connection : connections)
        {
            graph.removeConnection(connection);
        }
        connections.clear();
    }

    juce::AudioProcessorGraph::Node::Ptr getNode(juce::AudioProcessorGraph::NodeID nodeId) const
    {
        return graph.getNodeForId(nodeId);
    }

    juce::AudioProcessorGraph::Node::Ptr addNode(std::unique_ptr<juce::AudioProcessor> processor)
    {
        auto node = graph.addNode(std::move(processor));
        
        if (node == nullptr)
        {
            DBG("Failed to add node to graph.");
            throw std::runtime_error("Failed to add node to graph.");
        }

        return node;
    }

    juce::AudioProcessorGraph::Node::Ptr removeNode(juce::AudioProcessorGraph::NodeID nodeId)
    {
        return graph.removeNode(nodeId);
    }

    bool addConnection(
        const juce::AudioProcessorGraph::Connection& connection)
    {
        connections.push_back(connection);
        return graph.addConnection(connection);
    }

    bool removeConnection(
        const juce::AudioProcessorGraph::Connection& connection)
    {
        return graph.removeConnection(connection);
    }

    const juce::AudioProcessorGraph& getGraph() const noexcept { return graph; }

    const juce::AudioProcessorGraph::Node::Ptr getInputNode() const noexcept { return inputNode; }

    const juce::AudioProcessorGraph::Node::Ptr getOutputNode() const noexcept { return outputNode; }

    int getInputChannels() const noexcept { return inputChannels; }
    int getOutputChannels() const noexcept { return outputChannels; }

private:
    juce::AudioProcessorGraph::Node::Ptr inputNode;
    juce::AudioProcessorGraph::Node::Ptr outputNode;
    juce::AudioProcessorGraph graph;

    std::vector<juce::AudioProcessorGraph::Connection> connections;

    int inputChannels = 0;
    int outputChannels = 0;
};
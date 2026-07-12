#include <juce_core/juce_core.h>

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

    juce::AudioProcessorGraph& getGraph() { return graph; }
    juce::AudioProcessorGraph::Node::Ptr getInputNode() const { return inputNode; }
    juce::AudioProcessorGraph::Node::Ptr getOutputNode() const { return outputNode; }

    int getInputChannels() const { return inputChannels; }
    int getOutputChannels() const { return outputChannels; }

private:
    juce::AudioProcessorGraph::Node::Ptr inputNode;
    juce::AudioProcessorGraph::Node::Ptr outputNode;
    juce::AudioProcessorGraph graph;
    int inputChannels = 0;
    int outputChannels = 0;
};
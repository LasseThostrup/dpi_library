#pragma once

#include "FlowHandle.h"
#include "../memory/BufferWriter.h"

namespace dpi
{
    
template <typename Tuple>
struct TargetOutput
{
    Tuple* outputBuffer;
    BufferWriterBW* bufferWriter;
    size_t counter;
    TargetOutput(){};
    TargetOutput(Tuple* outputBuffer, BufferWriterBW* bufferWriter, size_t counter) : outputBuffer(outputBuffer), bufferWriter(bufferWriter), counter(counter) {};
};

template <typename Tuple, typename K>
class FlowSource
{

public:
    /**
     * @brief Construct a new Flow Source object
     * 
     * @param flowHandle - FlowHandle of the flow
     * @param sendTuplesCount - Number of tuples to be accumulated before sending over the network. In general: low number = small latency, high number = high BW
     */
    FlowSource(FlowHandle *flowHandle, size_t sendTuplesCount, K (*f_group)(Tuple), TargetID (*f_dist)(K));
    ~FlowSource();

    bool inject(Tuple tuple);
    void close();
        //Close flushes the intermediate buffers and calls close on all BufferWriters
private:
    K (*f_group)(Tuple);
    TargetID (*f_dist)(K);
    FlowHandle *m_flowHandle;
    std::unordered_map<TargetID, TargetOutput<Tuple>*> m_targetOutputs;
    size_t m_sendTuplesCount = 0;
    RDMAClient *m_rdmaClient = nullptr;
    bool m_isClosed = false;
};


template <typename Tuple, typename K>
FlowSource<Tuple, K>::FlowSource(FlowHandle *flowHandle, size_t sendTuplesCount, K (*f_group)(Tuple), TargetID (*f_dist)(K))
{
    std::cout << "Creating FlowSource, target size: " << flowHandle->targets.size() << '\n';
    if (sendTuplesCount == 0)
    {
        Logging::error(__FILE__, __LINE__, "sendTuplesCount parameter must be greater than 0.");
        return;
    }

    std::cout << "Creating RDMAClient" << '\n';
    m_rdmaClient = new RDMAClient((Config::DPI_INTERNAL_BUFFER_SIZE + sizeof(Config::DPI_SEGMENT_HEADER_t)) * flowHandle->targets.size());

    //Create BufferWriters and allocate memory for m_outputBuffers
    for(auto targetId : flowHandle->targets)
    {
        std::cout << "Connecting to remote FlowTarget with id " << targetId << '\n';
        m_rdmaClient->connect(Config::getIPFromNodeId(targetId), targetId);
        auto bufferName = Config::getBufferName(flowHandle->name, targetId);
        auto bufWriter = new BufferWriterBW(bufferName, new RegistryClient(), Config::DPI_INTERNAL_BUFFER_SIZE, m_rdmaClient);
        m_targetOutputs[targetId] = new TargetOutput<Tuple>(new Tuple[sendTuplesCount], bufWriter, 0);
        std::cout << "BufferWriter and output buffer created for TargetID: " << targetId << '\n';
    }

    this->f_group = f_group;
    this->f_dist = f_dist;
    this->m_sendTuplesCount = sendTuplesCount;
    this->m_flowHandle = flowHandle;
}

template <typename Tuple, typename K>
FlowSource<Tuple, K>::~FlowSource()
{
    if (!m_isClosed)
        close();

    delete m_rdmaClient;
}


template <typename Tuple, typename K>
bool FlowSource<Tuple, K>::inject(Tuple tuple)
{
    K key = f_group(tuple);
    TargetID targetId = f_dist(key);

    // std::cout << "Writing to output buffer for TargetID: " << target << '\n';

    auto &targetOutput = m_targetOutputs[targetId];
    targetOutput->outputBuffer[targetOutput->counter] = tuple;
    
    ++targetOutput->counter;
    // std::cout << "outputCounter: " << outputCounter << '\n';
    //Output buffer full --> append to remote buffer
    if (targetOutput->counter == m_sendTuplesCount)
    {
        // std::cout << "Appending "<< m_sendTuplesCount * sizeof(Tuple)<< " bytes" << '\n';
        targetOutput->counter = 0;
        return targetOutput->bufferWriter->append((void*)targetOutput->outputBuffer, m_sendTuplesCount * sizeof(Tuple));
    }
    return true;
}

template <typename Tuple, typename K>
void FlowSource<Tuple, K>::close()
{
    for(auto targetId : m_flowHandle->targets)
    {
        std::cout << "Closing BufferWriter on TargetID: " << targetId << '\n';
        auto &targetOutput = m_targetOutputs[targetId];
        if (targetOutput->counter > 0)
        {
            targetOutput->bufferWriter->append((void*)targetOutput->outputBuffer, targetOutput->counter * sizeof(Tuple));
        }
        targetOutput->bufferWriter->close();
    }
    m_isClosed = true;
}

} //dpi namespace end


/**
 * What does the source need?
 *  It needs BufferWriters to write to the respective buffers
 *  Need to have an rdma-client and share it amongst the BufferWriters
 * 
 * What does it need from the BufferHandle?
 *  It needs TargetIds...
 * 
 */
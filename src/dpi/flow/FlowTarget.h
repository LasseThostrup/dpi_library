#pragma once

#include "../../utils/Config.h"
#include "../memory/local_iterators/BufferIterator.h"
#include "../memory/NodeServer.h"
#include "../memory/RegistryClient.h"
#include "FlowHandle.h"

namespace dpi
{

template <typename Tuple>
class FlowTarget : public NodeServer
{
public:
    FlowTarget(TargetID targetID, string& flowName, uint16_t port = Config::DPI_NODE_PORT);

    void initializeConsume();
    Tuple* consume(size_t &returnedTuples, bool freeLastTuples);
//templated tuple
    //Tuple[] consume(size_t &returnedTuples, bool freeLastTuples)
        //Consume uses the iterator to get next segment --> create list with tuples (by copying the data)

private:
    TargetID targetID;
    string flowName;
    BufferIterator bufferIterator;
};

template <typename Tuple>
FlowTarget<Tuple>::FlowTarget(TargetID targetID, string& flowName, uint16_t port) : NodeServer(port)
{
    this->flowName = flowName;
    this->targetID = targetID;
}

template <typename Tuple>
void FlowTarget<Tuple>::initializeConsume()
{
    auto regClient = new RegistryClient();
    auto bufferName = Config::getBufferName(flowName, targetID);
    auto bufferHandle = regClient->retrieveBuffer(bufferName);
    bufferIterator = bufferHandle->getIterator((char*)this->getBuffer());

    delete regClient;
}

template <typename Tuple>
Tuple* FlowTarget<Tuple>::consume(size_t &returnedTuples, bool freeLastTuples)
{
    (void) freeLastTuples;
    if (bufferIterator.has_next())
    {
        size_t size = 0;
        auto dataPtr = bufferIterator.next(size);
        // std::cout << "consume - next returned a size of " << size << " sizeof(Tuple): " << sizeof(Tuple) << '\n';
        returnedTuples = size / sizeof(Tuple);
        return (Tuple*)dataPtr;
    }
    return nullptr;
}

} //dpi namespace end
#include "FlowRegistryServer.h"

FlowRegistryServer::FlowRegistryServer() : RegistryServer()
{

};
FlowRegistryServer::~FlowRegistryServer()
{

};

void FlowRegistryServer::handle(Any *sendMsg, Any *respMsg)
{

    if (sendMsg->Is<DPICreateFlowRequest>())
    {
        Logging::debug(__FILE__, __LINE__, "Got DPIAllocSegmentsRequest msg");
        bool success = true;
        DPICreateFlowRequest reqMsgUnpacked;
        DPICreateFlowResponse respMsgUnpacked;

        sendMsg->UnpackTo(&reqMsgUnpacked);
        string flowName = reqMsgUnpacked.name();
        FlowHandle flowHandle(flowName);
        size_t numberOfSources = reqMsgUnpacked.number_of_sources();
        for (size_t i = 0; i < (size_t)reqMsgUnpacked.targets_size(); i++)
        {
            size_t targetId = reqMsgUnpacked.targets(i);
            flowHandle.targets.push_back(targetId);

            //Create the DPI Buffers
            string bufferName = flowName + to_string(targetId);
            BufferHandle bufferHandle(bufferName, targetId, Config::DPI_SEGMENTS_PER_RING, numberOfSources, Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t));
            Any sendAny = MessageTypes::createDPIRegisterBufferRequest(bufferHandle);
            Any rcvAny; 
            RegistryServer::handle(&sendAny, &rcvAny); //Forward message to RegistryServer
            DPIAppendBufferResponse appendBufferResp;
            rcvAny.UnpackTo(&appendBufferResp);
            if (appendBufferResp.return_() != MessageErrors::NO_ERROR)
            {
                Logging::error(__FILE__, __LINE__, "Could not create buffer for flow!");
                success = false;
            }
        }

        flowHandles[flowName] = flowHandle;
        if (success)
            respMsgUnpacked.set_return_(MessageErrors::NO_ERROR);
        else
            respMsgUnpacked.set_return_(MessageErrors::DPI_CREATE_FLOW_FAILED);

        respMsg->PackFrom(respMsgUnpacked);
    }
    else
    {
        RegistryServer::handle(sendMsg, respMsg);
    }
};

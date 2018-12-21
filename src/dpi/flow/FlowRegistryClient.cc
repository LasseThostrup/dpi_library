#include "FlowRegistryClient.h"


FlowRegistryClient::FlowRegistryClient() : RegistryClient()
{

}


FlowHandle *FlowRegistryClient::createFlow(string &name, std::vector<TargetID> &targets, size_t numberOfSources)
{
    Any sendAny = MessageTypes::createDPICreateFlowRequest(name, targets, numberOfSources);
    Any rcvAny;

    if (!this->send(&sendAny, &rcvAny))
    {
        Logging::error(__FILE__, __LINE__, "Can not send message");
        return nullptr;
    }

    if (!rcvAny.Is<DPICreateFlowResponse>())
    {
        Logging::error(__FILE__, __LINE__, "Unexpected response");
        return nullptr;
    }

    Logging::debug(__FILE__, __LINE__, "Received response");
    DPICreateFlowResponse createFlowResp;
    rcvAny.UnpackTo(&createFlowResp);
    if (createFlowResp.return_() != MessageErrors::NO_ERROR)
    {
        Logging::error(__FILE__, __LINE__, "Error on server side");
        return nullptr;
    }
    auto flowHandle = new FlowHandle(name);
    flowHandle->targets = targets;
    std::cout << "Returning flowhandle with target.size() " << flowHandle->targets.size() << '\n';
    return flowHandle;
}

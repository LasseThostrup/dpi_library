#pragma once

#include "../../utils/Config.h"
#include "../memory/RegistryClient.h"
#include "FlowHandle.h"

namespace dpi
{
    
class FlowRegistryClient : public RegistryClient
{
public:

    FlowRegistryClient();
    FlowHandle *createFlow(string &name, std::vector<TargetID> &targets, size_t numberOfSources);

    //retrieve FlowHandle
private:

};


} //dpi namespace end
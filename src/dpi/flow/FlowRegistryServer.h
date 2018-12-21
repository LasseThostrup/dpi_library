#pragma once

#include "../../utils/Config.h"
#include "../memory/RegistryServer.h"
#include "FlowHandle.h"


namespace dpi
{
    
class FlowRegistryServer : public RegistryServer
{
public:
    // Constructors and Destructors
    FlowRegistryServer();
    FlowRegistryServer(FlowRegistryServer &&) = default;
    FlowRegistryServer(const FlowRegistryServer &) = default;
    FlowRegistryServer &operator=(FlowRegistryServer &&) = default;
    FlowRegistryServer &operator=(const FlowRegistryServer &) = default;
    ~FlowRegistryServer();

    /**
     * @brief handle messages 
     *  
     * @param sendMsg the incoming message
     * @param respMsg the response message
     */
    void handle(Any *sendMsg, Any *respMsg);

private:
    std::map<string, FlowHandle> flowHandles;
};


} //dpi namespace end




/** Responsibility of FlowRegistryServer:
 *  On creation of flow: create a buffer on each target by creating protobuf msg and calling RegistryServer::handle
 *  
 *  
 *  
 *  
 * Questions:
 *  How to specify wanted size of rings and segments? --> start by pre configured values
 * 
 */
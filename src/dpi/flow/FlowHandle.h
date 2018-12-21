#pragma once

#include "../../utils/Config.h"

typedef uint64_t SourceID;
typedef uint64_t TargetID;

namespace dpi
{
    
struct FlowHandle
{
    string name; //Used to identify the flow
    std::vector<TargetID> targets;

    FlowHandle() {}
    FlowHandle(string name) : name(name) {}
};


} //dpi namespace end
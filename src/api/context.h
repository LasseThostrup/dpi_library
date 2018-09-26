/**
 * @file context.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */

#pragma once

#include "../utils/Config.h"
#include <unordered_map>
#include "../dpi/BufferWriter.h"
#include "../dpi/BufferReader.h"
#include "../dpi/RegistryClient.h"

/**
 * @brief DPI_Context containing the objects needed for the different DPI operations
 */
struct DPI_Context
{
    #ifdef DPI_STRATEGY_PRIVATE //todo: remove ugly hack
    std::unordered_map<std::string, BufferWriter<BufferWriterPrivate>*> buffer_writers;
    #endif
    #ifdef DPI_STRATEGY_SHARED
    std::unordered_map<std::string, BufferWriter<BufferWriterShared>*> buffer_writers;
    #endif

    std::unordered_map<std::string, BufferReader*> buffer_readers;

    RegistryClient* registry_client = nullptr;
};
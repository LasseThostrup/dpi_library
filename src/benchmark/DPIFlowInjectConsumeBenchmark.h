/**
 * @file DPIFlowInjectConsumeBenchmark_H_.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-12-21
 */

#ifndef DPIFlowInjectConsumeBenchmark_H_
#define DPIFlowInjectConsumeBenchmark_H_

#include "../utils/Config.h"
#include "../utils/StringHelper.h"
#include "../thread/Thread.h"
#include "../net/rdma/RDMAClient.h"
#include "../dpi/memory/NodeServer.h"
#include "../dpi/memory/RegistryServer.h"
#include "../dpi/memory/BufferWriter.h"
#include "../dpi/flow/FlowTarget.h"
#include "../dpi/flow/FlowSource.h"
#include "../dpi/flow/FlowHandle.h"
#include "../dpi/flow/FlowRegistryClient.h"
#include "../dpi/flow/FlowRegistryServer.h"
#include "BenchmarkRunner.h"

#include <vector>
#include <mutex>
#include <condition_variable>
#include <iostream>

namespace dpi
{


struct TestTuple
{
    int64_t key;
    int64_t value;
    TestTuple(){};
    TestTuple(int64_t key, int64_t value) : key(key), value(value){};
};


class DPIFlowInjectConsumeBenchmarkBarrier : public BufferIteratorLat
{


public:

  DPIFlowInjectConsumeBenchmarkBarrier(char *rdmaBufferPtr, std::vector<BufferSegment> &entrySegments, string &bufferName) 
  : BufferIteratorLat(rdmaBufferPtr, entrySegments, bufferName){};


bool create(){return manipulateBarrier(true);}
bool release(){return manipulateBarrier(false);}


private:

  bool manipulateBarrier(bool barrier){
    {
        if (segment_iterators.empty())
        {
            return false;
        }

        while (pointer_to_iter == segment_iterators.end())
        {
            {
                if (!(*pointer_to_iter).iter->isConsumable())
                {
                    //advance iter to next ring
                    (*pointer_to_iter).prev->setWriteable(barrier);
                    pointer_to_iter++;
                    // std::cout << "Segment in ring was not consumable, advancing" << '\n';
                }
            }
        }
        return true;
    }
  }

};

class DPIFlowInjectConsumeBenchmarkThread : public Thread
{

public:
  DPIFlowInjectConsumeBenchmarkThread(NodeID nodeid, string &conns, size_t size, size_t iter, size_t numberAppenders, bool signaled);
  ~DPIFlowInjectConsumeBenchmarkThread();
  void run();
  bool ready()
  {
    return m_ready;
  }

private:
  string flowName = "flow_benchmark";
  bool m_ready = false;

  FlowSource<TestTuple, int64_t> *m_flowSource = nullptr;
  FlowRegistryClient *m_regClient = nullptr;
  size_t numberSegments = 50;
  void *m_data;
  size_t m_size;
  size_t m_iter;
  bool m_sendSignaled = false;
  string m_conns;
  size_t m_threadId;
  int m_ctr = 0;
};

class DPIFlowInjectConsumeBenchmark : public BenchmarkRunner
{
public:
  DPIFlowInjectConsumeBenchmark(config_t config, bool isClient);

  DPIFlowInjectConsumeBenchmark(string &region, size_t serverPort, size_t size, size_t iter,
                      size_t threads);

  ~DPIFlowInjectConsumeBenchmark();

  void printHeader()
  {
    cout << "Size\tIter\tBW (MB)\tmopsPerS\tTime (micro sec.)" << endl;
  }

  void printResults()
  {
    double time = (this->time()) / (1e9);
    size_t bw = (((double)m_size * m_iter * m_numThreads) / (1024 * 1024)) / time;
    double mops = (((double)m_iter * m_numThreads) / time) / (1e6);
    double ms_time = (this->time()) / (1e3);

    cout << m_size << "\t" << m_iter << "\t" << bw << "\t" << mops << "\t" << ms_time << endl;
  }


  void printUsage()
  {
    cout << "istore2_perftest ... -s #servers ";
    cout << "(-d #dataSize -p #serverPort -t #threadNum)?" << endl;
  }

  void runClient();
  void runServer();
  double time();

  static mutex waitLock;
  static condition_variable waitCv;
  static bool signaled;

private:
  size_t m_serverPort;
  size_t m_size;
  size_t m_iter;
  size_t m_numThreads;

  bool m_sendSignaled = false;

  int numberSegments;

  string m_conns;
  string flowName = "flow_benchmark";

  vector<DPIFlowInjectConsumeBenchmarkThread *> m_threads;

  FlowTarget<TestTuple> *m_flowTarget;
  FlowRegistryServer *m_regServer;
};

} // namespace dpi

#endif /* DPIFlowInjectConsumeBenchmark_H_*/

#pragma once

#include "../../utils/Config.h"

#include "../../dpi/NodeServer.h" 
#include "../../dpi/NodeClient.h"
#include "../../dpi/RegistryClient.h"
#include "../../dpi/BufferWriter.h"

#include <atomic>

class TestBufferWriter : public CppUnit::TestFixture {
DPI_UNIT_TEST_SUITE(TestBufferWriter);
  DPI_UNIT_TEST(testBuffer);
  DPI_UNIT_TEST(testAppendPrivate_SingleInts);
  DPI_UNIT_TEST(testAppendPrivate_SplitData);
  DPI_UNIT_TEST(testAppendPrivate_SimpleData);
  DPI_UNIT_TEST(testAppendPrivate_MultipleConcurrentClients);
  DPI_UNIT_TEST(testAppendPrivate_VaryingDataSizes);
  DPI_UNIT_TEST(testAppendShared_SimpleData);
  DPI_UNIT_TEST(testAppendShared_AtomicHeaderManipulation);
  DPI_UNIT_TEST(testAppendShared_MultipleConcurrentClients);  
  DPI_UNIT_TEST(testAppendShared_VaryingDataSizes);  
DPI_UNIT_TEST_SUITE_END();
 
 public:
  void setUp();
  void tearDown();
 
  // Private Strategy
  void testBuffer();
  void testAppendPrivate_SingleInts();
  void testAppendPrivate_SplitData();
  void testAppendPrivate_SimpleData();
  void testAppendPrivate_MultipleConcurrentClients();
  void testAppendPrivate_VaryingDataSizes();

  // Shared Strategy
  void testAppendShared_SimpleData();
  void testAppendShared_AtomicHeaderManipulation();
  void testAppendShared_MultipleConcurrentClients();
  void testAppendShared_VaryingDataSizes();


  static std::atomic<int> bar;    // Counter of threads, faced barrier.
  static std::atomic<int> passed; // Number of barriers, passed by all threads.
  static const int NUMBER_THREADS = 2;


private:
  void* readSegmentData(BufferSegment* segment, size_t &size);
  Config::DPI_SEGMENT_HEADER_t *readSegmentHeader(BufferSegment* segment);

  NodeClient* m_nodeClient;
  NodeServer* m_nodeServer;
  RegistryClient* m_stub_regClient;

class RegistryClientStub : public RegistryClient
{
public:

  BufferHandle* createBuffer(string& name, NodeID node_id, size_t size, size_t threshold)
  {
    (void) name;
    (void) size;
    (void) threshold;
    
    m_buffHandle = new BufferHandle(name, node_id);
    RDMAClient *rdmaClient = new RDMAClient();
    rdmaClient->connect(Config::getIPFromNodeId(node_id));
    size_t remoteOffset = 0;
    rdmaClient->remoteAlloc(Config::getIPFromNodeId(node_id), Config::DPI_SEGMENT_SIZE, remoteOffset);
    
    BufferSegment seg;
    seg.offset = remoteOffset;
    seg.size = Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t);
    seg.threshold = Config::DPI_SEGMENT_SIZE * Config::DPI_SEGMENT_THRESHOLD_FACTOR;
    appendSegment(name, seg);
    
    // std::cout << "Created buffer" << '\n';
    
    return m_buffHandle;
  }

  bool registerBuffer(BufferHandle* handle)
  {
    // std::cout << "Register Buffer" << '\n';
    BufferHandle* copy_buffHandle = new BufferHandle(handle->name, handle->node_id);
    for(auto segment : handle->segments){
      copy_buffHandle->segments.push_back(segment); 
    }
    m_buffHandle = copy_buffHandle;
    return true;
  }
  BufferHandle* retrieveBuffer(string& name)
  {
    // std::cout << "Retrieve Buffer" << '\n';
    (void) name;
    //Copy a new BufferHandle to emulate distributed setting (Or else one nodes changes to the BufferHandle would affect another nodes BufferHandle without retrieving the buffer first)
    BufferHandle*  copy_buffHandle = new BufferHandle(m_buffHandle->name, m_buffHandle->node_id);
    for(auto segment : m_buffHandle->segments){
      copy_buffHandle->segments.push_back(segment); 
    }
    return copy_buffHandle;
  }
  bool appendSegment(string& name, BufferSegment& segment)
  {
    // std::cout << "Appending segment to buffer" << '\n';
    //Implement locking if stub should support concurrent appending of segments.
    (void) name;
    appendSegMutex.lock();
    BufferSegment seg;
    seg.offset = segment.offset;
    seg.size = segment.size;
    seg.threshold = segment.threshold;
    m_buffHandle->segments.push_back(seg);
    appendSegMutex.unlock();
    
    return true;
  }

private:
  BufferHandle* m_buffHandle = nullptr; //For this stub we just have one buffHandle
  std::mutex appendSegMutex;
};
 
struct TestData
{
  int a;
  int b;
  int c;
  int d;
  TestData(int a, int b, int c, int d) : a(a), b(b), c(c), d(d){}
};

template <class DataType, class Strategy>
class BufferWriterClient : public Thread
{
  NodeServer* nodeServer = nullptr;
  RegistryClient* regClient = nullptr;
  BufferHandle* buffHandle = nullptr;
  std::vector<DataType> *dataToWrite = nullptr; //tuple<ptr to data, size in bytes>

public: 

  BufferWriterClient(NodeServer* nodeServer, RegistryClient* regClient, BufferHandle* buffHandle, std::vector<DataType> *dataToWrite) : 
    Thread(), nodeServer(nodeServer), regClient(regClient), buffHandle(buffHandle), dataToWrite(dataToWrite) {}

  void run() 
  {
    //ARRANGE

    BufferWriter<Strategy> buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, regClient);

    barrier_wait();

    //ACT
    for(size_t i = 0; i < dataToWrite->size(); i++)
    {
      buffWriter.append(&dataToWrite->operator[](i), sizeof(DataType));
    }

    buffWriter.close();
  }

    void barrier_wait()
    {
        std::cout << "Enter Barrier" << '\n';
        int passed_old = passed.load(std::memory_order_relaxed);

        if (bar.fetch_add(1) == (NUMBER_THREADS - 1))
        {
            // The last thread, faced barrier.
            bar = 0;
            // Synchronize and store in one operation.
            passed.store(passed_old + 1, std::memory_order_release);
        }
        else
        {
            // Not the last thread. Wait others.
            while (passed.load(std::memory_order_relaxed) == passed_old)
            {
            };
            // Need to synchronize cache with other threads, passed barrier.
            std::atomic_thread_fence(std::memory_order_acquire);
        }
    }
};

};

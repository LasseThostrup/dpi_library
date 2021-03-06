/**
 * @file IntegrationTestsAppend.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */

#pragma once

#include "../../utils/Config.h"

#include "../../dpi/RegistryClient.h"
#include "../../dpi/RegistryServer.h"
#include "../../dpi/BufferHandle.h"
#include "../../dpi/NodeServer.h"
#include "../../dpi/NodeClient.h"
#include "../../dpi/RegistryClient.h"
#include "../../dpi/BufferWriter.h"
#include "../../dpi/BufferConsumer.h"

#include <atomic>


class TestBufferIterator : public CppUnit::TestFixture
{
  DPI_UNIT_TEST_SUITE(TestBufferIterator);
    DPI_UNIT_TEST(testBufferIteratorFourAppender_Interleaved);
    // DPI_UNIT_TEST(AppenderConsumerBenchmark);
    DPI_UNIT_TEST(testSegmentIterator);
    DPI_UNIT_TEST(testBufferIterator);
    DPI_UNIT_TEST(testBufferIteratorFourAppender_NotInterleaved);
    DPI_UNIT_TEST(testBufferIteratorSegmentsSameSizeAsMsgs_NotInterleaved);
    DPI_UNIT_TEST(testBufferIteratorFourAppenderSegSizeSameAsMsg_Interleaved);
  DPI_UNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  // void FourAppendersOneConsumerInterleaved_DontReuseSegs();
  // void FourAppendersOneConsumerInterleaved_ReuseSegs();
  // void AppenderConsumerBenchmark();

  void testBufferIteratorFourAppender_Interleaved();
  void testBufferIteratorFourAppender_NotInterleaved();
  void testBufferIteratorFourAppenderSegSizeSameAsMsg_Interleaved();
  void testBufferIteratorSegmentsSameSizeAsMsgs_NotInterleaved();
  void testSegmentIterator();
  void testBufferIterator();

  static std::atomic<int> bar;    // Counter of threads, faced barrier.
  static std::atomic<int> passed; // Number of barriers, passed by all threads.

private:
  RegistryClient *m_regClient;
  RegistryServer *m_regServer;
  NodeServer *m_nodeServer; 
  NodeClient *m_nodeClient; 



template <class DataType> 
class BufferWriterClient : public Thread
{
  BufferHandle* buffHandle = nullptr;
  string& bufferName = "";
  std::vector<DataType> *dataToWrite = nullptr; //tuple<ptr to data, size in bytes>

public: 

  BufferWriterClient(string& bufferName, std::vector<DataType> *dataToWrite, int numThread = 4) : 
    Thread(), bufferName(bufferName), dataToWrite(dataToWrite), NUMBER_THREADS(numThread) {}

  int NUMBER_THREADS;
  
  void run() 
  {
    //ARRANGE

    BufferWriterBW buffWriter(bufferName, new RegistryClient(), Config::DPI_INTERNAL_BUFFER_SIZE);

    barrier_wait(); //Use barrier to simulate concurrent appends between BufferWriters

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
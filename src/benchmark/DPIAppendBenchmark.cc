

#include "DPIAppendBenchmark.h"

mutex DPIAppendBenchmark::waitLock;
condition_variable DPIAppendBenchmark::waitCv;
bool DPIAppendBenchmark::signaled;

DPIAppendBenchmarkThread::DPIAppendBenchmarkThread(NodeID nodeid, string &conns,
                                                   size_t size, size_t iter, size_t numberAppenders)
{
  m_size = size;
  m_iter = iter;
  m_nodeId = nodeid;
  m_conns = conns;
  
  BufferHandle *buffHandle;
  m_regClient = new RegistryClient();

  if (m_nodeId == 1)
  {
    buffHandle = new BufferHandle(bufferName, m_nodeId, 10, numberAppenders, Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t));
    m_regClient->registerBuffer(buffHandle);
  }

  m_bufferWriter = new BufferWriterBW(bufferName, m_regClient, Config::DPI_INTERNAL_BUFFER_SIZE);
  std::cout << "DPI append Thread constructed" << '\n';
}

DPIAppendBenchmarkThread::~DPIAppendBenchmarkThread()
{
}

void DPIAppendBenchmarkThread::run()
{

  std::cout << "DPI append Thread running" << '\n';
  unique_lock<mutex> lck(DPIAppendBenchmark::waitLock);
  if (!DPIAppendBenchmark::signaled)
  {
    m_ready = true;
    DPIAppendBenchmark::waitCv.wait(lck);
  }
  lck.unlock();

  std::cout << "Benchmark Started" << '\n';
  void *scratchPad = malloc(m_size);
  memset(scratchPad, 1, m_size);

  startTimer();
  for (size_t i = 0; i <= m_iter; i++)
  {
    m_bufferWriter->append(scratchPad, m_size);
  }

  m_bufferWriter->close();
  endTimer();
}

DPIAppendBenchmark::DPIAppendBenchmark(config_t config, bool isClient)
    : DPIAppendBenchmark(config.server, config.port, config.data, config.iter,
                         config.threads)
{

  Config::DPI_NODES.clear();
  string dpiServerNode = config.server + ":" + to_string(config.port);
  Config::DPI_NODES.push_back(dpiServerNode);
  std::cout << "connection: " << Config::DPI_NODES.back() << '\n';
  Config::DPI_REGISTRY_SERVER = config.registryServer;
  Config::DPI_REGISTRY_PORT = config.registryPort;
  Config::DPI_INTERNAL_BUFFER_SIZE = config.internalBufSize;

  this->isClient(isClient);

  //check parameters
  if (isClient && config.server.length() > 0)
  {
    std::cout << "Starting DPIAppend Clients" << '\n';
    std::cout << "Internal buf size: " << Config::DPI_INTERNAL_BUFFER_SIZE << '\n';
    this->isRunnable(true);
  }
  else if (!isClient)
  {
    this->isRunnable(true);
  }
}

DPIAppendBenchmark::DPIAppendBenchmark(string &conns, size_t serverPort,
                                       size_t size, size_t iter, size_t threads)
{
  m_conns = conns;
  m_serverPort = serverPort;
  m_size = size;
  m_iter = iter;
  m_numThreads = threads;
  DPIAppendBenchmark::signaled = false;

  std::cout << "Iterations in client" << m_iter << '\n';
  std::cout << "With " << m_size << " byte appends" << '\n';
}

DPIAppendBenchmark::~DPIAppendBenchmark()
{
  if (this->isClient())
  {
    for (size_t i = 0; i < m_threads.size(); i++)
    {
      delete m_threads[i];
    }
    m_threads.clear();
  }
  else
  {
    if (m_nodeServer != nullptr)
    {
      std::cout << "stopping node server" << '\n';
      m_nodeServer->stopServer();
      delete m_nodeServer;
    }
    m_nodeServer = nullptr;

    if (m_regServer != nullptr)
    {
      m_regServer->stopServer();
      delete m_regServer;
    }
    m_regServer = nullptr;
  }
}

void DPIAppendBenchmark::runServer()
{
  std::cout << "Starting DPI Server port " << m_serverPort << '\n';

  m_nodeServer = new NodeServer(m_serverPort);

  if (!m_nodeServer->startServer())
  {
    throw invalid_argument("DPIAppendBenchmark could not start NodeServer!");
  }

  m_regServer = new RegistryServer();

  if (!m_regServer->startServer())
  {
    throw invalid_argument("DPIAppendBenchmark could not start RegistryServer!");
  }


  auto m_regClient = new RegistryClient();
  usleep(Config::DPI_SLEEP_INTERVAL);

  while (nullptr == m_regClient->retrieveBuffer(bufferName))
  {
    usleep(Config::DPI_SLEEP_INTERVAL * 5);
  }

  auto handle_ret = m_regClient->retrieveBuffer(bufferName);


  BufferIterator bufferIterator = handle_ret->getIteratorLat((char *)m_nodeServer->getBuffer());
  // DPILatencyBenchmarkBarrier barrier((char *)m_nodeServer->getBuffer(), handle_ret->entrySegments,bufferName);
  
  // barrier.create();
  std::cout << "Client Barrier created" << '\n';
  waitForUser();
  // barrier.release();
  int count = 0;
  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

  // bufferIterator.has_next();
  // std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
  while (bufferIterator.has_next())
  {
    // std::cout << "Interator Segment" << '\n';
    size_t dataSize;
    int *data = (int *)bufferIterator.next(dataSize);
    for (size_t i = 0; i < dataSize / m_size; i++, data++)
    {
      count++;
    }
  }
  std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  std::cout << "Benchmark took " << duration << "microseconds" << std::endl;
  std::cout << "Number of msgs " << count << std::endl;

}

void DPIAppendBenchmark::runClient()
{
  //start all client threads
  for (size_t i = 1; i <= m_numThreads; i++)
  {
    DPIAppendBenchmarkThread *perfThread = new DPIAppendBenchmarkThread(i, m_conns,
                                                                        m_size,
                                                                        m_iter, m_numThreads);
    perfThread->start();
    if (!perfThread->ready())
    {
      usleep(Config::DPI_SLEEP_INTERVAL);
    }
    m_threads.push_back(perfThread);
  }

  //wait for user input
  waitForUser();
  // usleep(10000000);

  //send signal to run benchmark
  DPIAppendBenchmark::signaled = false;
  unique_lock<mutex> lck(DPIAppendBenchmark::waitLock);
  DPIAppendBenchmark::waitCv.notify_all();
  DPIAppendBenchmark::signaled = true;
  lck.unlock();
  for (size_t i = 0; i < m_threads.size(); i++)
  {
    m_threads[i]->join();
  }
}

double DPIAppendBenchmark::time()
{
  uint128_t totalTime = 0;
  for (size_t i = 0; i < m_threads.size(); i++)
  {
    totalTime += m_threads[i]->time();
  }
  return ((double)totalTime) / m_threads.size();
}

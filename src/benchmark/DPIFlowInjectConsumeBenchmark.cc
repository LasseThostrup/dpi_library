

#include "DPIFlowInjectConsumeBenchmark.h"

mutex DPIFlowInjectConsumeBenchmark::waitLock;
condition_variable DPIFlowInjectConsumeBenchmark::waitCv;
bool DPIFlowInjectConsumeBenchmark::signaled;

DPIFlowInjectConsumeBenchmarkThread::DPIFlowInjectConsumeBenchmarkThread(size_t threadId, string &conns,
                                                     size_t size, size_t iter, size_t numberAppenders, bool signaled)
{
  m_size = size;
  m_iter = iter;
  m_threadId = threadId;
  m_conns = conns;
  m_sendSignaled = signaled;
  std::cout << "/* m_size */" << m_size << '\n';
  std::cout << "/* iterations */" << iter << '\n';
  std::cout << "Sending Signaled " << m_sendSignaled << '\n';

  FlowHandle* flowHandle;
  m_regClient = new FlowRegistryClient();
  std::vector<TargetID> targets;
  targets.push_back(1);

  if (m_threadId == 1)
  {
    std::cout << "Creating Flow" << '\n';

    flowHandle = m_regClient->createFlow(flowName, targets, numberAppenders);
  }
  else
  {
    flowHandle = new FlowHandle(flowName);
    flowHandle->targets = targets;
  }

  auto f_group=[](TestTuple t){return t.key;};
  auto f_dist=[](int64_t k){return (TargetID)(1);};

  std::cout << "Thead: " << threadId << " initializing FlowSource " << '\n';
  m_flowSource = new FlowSource<TestTuple, int64_t>(flowHandle, m_size/sizeof(TestTuple), f_group, f_dist);
  std::cout << "DPI Flow Inject Consume Benchmark - Thread constructed" << '\n';
}

DPIFlowInjectConsumeBenchmarkThread::~DPIFlowInjectConsumeBenchmarkThread()
{
}

void DPIFlowInjectConsumeBenchmarkThread::run()
{

  std::cout << "DPI FlowSource Thread running" << '\n';
  unique_lock<mutex> lck(DPIFlowInjectConsumeBenchmark::waitLock);
  if (!DPIFlowInjectConsumeBenchmark::signaled)
  {
    m_ready = true;
    DPIFlowInjectConsumeBenchmark::waitCv.wait(lck);
  }
  lck.unlock();

  std::cout << "Benchmark Started" << '\n';
  size_t tuplesPerWrite = m_size/sizeof(TestTuple);
  TestTuple * tuples = new TestTuple[tuplesPerWrite] {TestTuple(1, 1)};

  startTimer();
  for (size_t i = 0; i <= m_iter; i++)
  {
    for (size_t j = 0; j < tuplesPerWrite; j++)
    {    
      m_flowSource->inject(tuples[j]);
    }
  }
  m_flowSource->close();

  endTimer();
}

DPIFlowInjectConsumeBenchmark::DPIFlowInjectConsumeBenchmark(config_t config, bool isClient)
    : DPIFlowInjectConsumeBenchmark(config.server, config.port, config.data, config.iter,
                          config.threads)
{

  Config::DPI_NODES.clear();
  string dpiServerNode = config.server + ":" + to_string(config.port);
  Config::DPI_NODES.push_back(dpiServerNode);
  std::cout << "connection: " << Config::DPI_NODES.back() << '\n';
  Config::DPI_REGISTRY_SERVER = config.registryServer;
  Config::DPI_REGISTRY_PORT = config.registryPort;
  Config::DPI_INTERNAL_BUFFER_SIZE = config.internalBufSize;

  m_sendSignaled = config.signaled;

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

DPIFlowInjectConsumeBenchmark::DPIFlowInjectConsumeBenchmark(string &conns, size_t serverPort,
                                         size_t size, size_t iter, size_t threads)
{
  m_conns = conns;
  m_serverPort = serverPort;
  m_size = size;
  m_iter = iter;
  m_numThreads = threads;
  DPIFlowInjectConsumeBenchmark::signaled = false;

  std::cout << "Iterations in client" << m_iter << '\n';
  std::cout << "With " << m_size << " byte appends" << '\n';
}

DPIFlowInjectConsumeBenchmark::~DPIFlowInjectConsumeBenchmark()
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
    if (m_flowTarget != nullptr)
    {
      std::cout << "stopping flow target" << '\n';
      m_flowTarget->stopServer();
      delete m_flowTarget;
    }
    m_flowTarget = nullptr;

    if (m_regServer != nullptr)
    {
      m_regServer->stopServer();
      delete m_regServer;
    }
    m_regServer = nullptr;
  }
}

void DPIFlowInjectConsumeBenchmark::runServer()
{
  std::cout << "Starting DPI Server port " << m_serverPort << '\n';

  m_flowTarget = new FlowTarget<TestTuple>(1, flowName, m_serverPort);

  if (!m_flowTarget->startServer())
  {
    throw invalid_argument("DPIFlowInjectConsumeBenchmark could not start FlowTarget!");
  }

  m_regServer = new FlowRegistryServer();

  if (!m_regServer->startServer())
  {
    throw invalid_argument("DPIFlowInjectConsumeBenchmark could not start RegistryServer!");
  }

  auto regClient = new RegistryClient();
  usleep(Config::DPI_SLEEP_INTERVAL);

  auto bufferName = Config::getBufferName(flowName, 1);
  while (nullptr == regClient->retrieveBuffer(bufferName));
  {
    usleep(Config::DPI_SLEEP_INTERVAL * 10);
  }

  auto handle_ret = regClient->retrieveBuffer(bufferName);
  DPIFlowInjectConsumeBenchmarkBarrier barrier((char *)m_flowTarget->getBuffer(), handle_ret->entrySegments,bufferName);
  
  barrier.create();
  std::cout << "Client Barrier created" << '\n';
  waitForUser();
  barrier.release();
  int count = 0;

  m_flowTarget->initializeConsume();

  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

  TestTuple* tuples = nullptr;
  size_t tupleCount = 0;
  int64_t expectedKey = 0;
  do
  {
      tuples = m_flowTarget->consume(tupleCount, true);
      // std::cout << "Consume gave " << tupleCount << " tuples." << '\n';
      if (tuples != nullptr)
      {
          for (size_t i = 0; i < tupleCount; i++)
          {
            ++count;
          }
      }
  } while (tuples != nullptr);

  std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  std::cout << "Consuming took " << duration << " us" << std::endl;
  std::cout << "Number of tuples consumed " << count << std::endl;
}

void DPIFlowInjectConsumeBenchmark::runClient()
{
  //start all client threads
  std::cout << "Starting " << m_numThreads << " client threads" << '\n';
  for (size_t i = 1; i <= m_numThreads; i++)
  {
    DPIFlowInjectConsumeBenchmarkThread *perfThread = new DPIFlowInjectConsumeBenchmarkThread(i, m_conns,
                                                                          m_size,
                                                                          m_iter, m_numThreads, m_sendSignaled);
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
  DPIFlowInjectConsumeBenchmark::signaled = false;
  unique_lock<mutex> lck(DPIFlowInjectConsumeBenchmark::waitLock);
  DPIFlowInjectConsumeBenchmark::waitCv.notify_all();
  DPIFlowInjectConsumeBenchmark::signaled = true;
  lck.unlock();
  for (size_t i = 0; i < m_threads.size(); i++)
  {
    m_threads[i]->join();
  }
}

double DPIFlowInjectConsumeBenchmark::time()
{
  uint128_t totalTime = 0;
  for (size_t i = 0; i < m_threads.size(); i++)
  {
    totalTime += m_threads[i]->time();
  }
  return ((double)totalTime) / m_threads.size();
}

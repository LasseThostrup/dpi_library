#include "TestFlowSource.h"


void TestFlowSource::setUp()
{  
    //Setup Test DPI
    Config::RDMA_MEMSIZE = 1024ul * 1024 * 1024 * 1;  //1GB
    Config::DPI_SEGMENT_SIZE = (2048 + sizeof(Config::DPI_SEGMENT_HEADER_t));
    Config::DPI_INTERNAL_BUFFER_SIZE = 4096;
    Config::DPI_REGISTRY_SERVER = "127.0.0.1";
    Config::DPI_REGISTRY_PORT = 5300;
    Config::DPI_NODE_PORT = 5400;
    Config::DPI_NODES.clear();
    string dpiTestNode = "127.0.0.1:" + to_string(Config::DPI_NODE_PORT);
    Config::DPI_NODES.push_back(dpiTestNode);
    Config::DPI_SEGMENTS_PER_RING = 50;

    std::cout << "Creating FlowTarget" << '\n';
    m_flowTarget = new FlowTarget<TestTuple>(1, flowName);

    std::cout << "Startomg FlowTarget" << '\n';
    m_flowTarget->startServer();

    std::cout << "Creating FlowRegistryServer" << '\n';
    m_regServer = new FlowRegistryServer();

    std::cout << "Starting FlowRegistryServer" << '\n';
    m_regServer->startServer();

    std::cout << "Starting FlowRegistryClient" << '\n';
    m_regClient = new FlowRegistryClient();

}


void TestFlowSource::tearDown()
{
    if (m_regServer != nullptr)
    {
        m_regServer->stopServer();
        delete m_regServer;
        m_regServer = nullptr;
    }
    if (m_flowTarget != nullptr)
    {
        m_flowTarget->stopServer();
        delete m_flowTarget;
        m_flowTarget = nullptr;
    }
    if (m_regClient != nullptr)
    {
        delete m_regClient;
        m_regClient = nullptr;
    }
}

void TestFlowSource::simpleInjectConsumeTest()
{
    std::cout << "simpleInjectConsumeTest" << '\n';
    

    std::vector<TargetID> targets;
    targets.push_back(1);
    std::cout << "Creating Flow" << '\n';
    FlowHandle* flowHandle = m_regClient->createFlow(flowName, targets, 1);

    auto f_group=[](TestTuple t){return t.key;};
    auto f_dist=[](int64_t k){return (TargetID)((k % 1) + 1);};

    std::cout << "Creating FlowSource" << '\n';
    FlowSource<TestTuple, int64_t> flowSource(flowHandle, 10, f_group, f_dist);
        
    for(int64_t i = 0; i < 25; i++)
    {
        std::cout << "Injection tuple with key: " << i << '\n';
        flowSource.inject(TestTuple(i, i));        
    }    
    
    flowSource.close();
    
    m_flowTarget->initializeConsume();

    TestTuple* tuples = nullptr;
    size_t tupleCount = 0;
    int64_t expectedKey = 0;
    do
    {
        tuples = m_flowTarget->consume(tupleCount, true);
        if (tuples != nullptr)
        {
            for (size_t i = 0; i < tupleCount; i++)
            {
                std::cout << "Consumed tuple with key: " << tuples[i].key << '\n';
                CPPUNIT_ASSERT_EQUAL_MESSAGE("Consumed key does not match injected", expectedKey, tuples[i].key);
                ++expectedKey;
            }
        }
    } while (tuples != nullptr);
}


void TestFlowSource::interleavedInjectConsumeTest()
{
    std::cout << "interleavedInjectConsumeTest" << '\n';
    

    std::vector<TargetID> targets;
    targets.push_back(1);
    std::cout << "Creating Flow" << '\n';
    FlowHandle* flowHandle = m_regClient->createFlow(flowName, targets, 1);

    auto f_group=[](TestTuple t){return t.key;};
    auto f_dist=[](int64_t k){return (TargetID)((k % 1) + 1);};


    auto flowTarget = m_flowTarget;
    std::thread consumer([flowTarget]() {
        flowTarget->initializeConsume();

        TestTuple* tuples = nullptr;
        size_t tupleCount = 0;
        int64_t expectedKey = 0;
        do
        {
            tuples = flowTarget->consume(tupleCount, true);
            // std::cout << "Consume gave " << tupleCount << " tuples." << '\n';
            if (tuples != nullptr)
            {
                for (size_t i = 0; i < tupleCount; i++)
                {
                    // std::cout << "Consumed tuple with key: " << tuples[i].key << '\n';
                    CPPUNIT_ASSERT_EQUAL_MESSAGE("Consumed key does not match injected", expectedKey, tuples[i].key);
                    ++expectedKey;
                }
            }
        } while (tuples != nullptr);
    });

    std::cout << "Creating FlowSource" << '\n';
    FlowSource<TestTuple, int64_t> flowSource(flowHandle, 100, f_group, f_dist);
    std::cout << "Injecting..." << '\n';
    for(int64_t i = 0; i < 100000; i++)
    {
        // std::cout << "Injection tuple with key: " << i << '\n';
        flowSource.inject(TestTuple(i, i));
    }    
    
    flowSource.close();
    
    consumer.join();
}
#include "TestRegistryClient.h"

void TestRegistryClient::setUp()
{
  m_nodeServer = new NodeServer();
  CPPUNIT_ASSERT(m_nodeServer->startServer());
  std::cout << "Start NodeServer" << '\n';
  m_regServer = new RegistryServer();
  std::cout << "Start RegServer" << '\n';
  CPPUNIT_ASSERT(m_regServer->startServer());
  std::cout << "Started NodeServer" << '\n';

  m_regClient = new RegistryClient();
  std::cout << "Start RegClient" << '\n';
}

void TestRegistryClient::tearDown()
{
  if (m_regServer != nullptr)
  {
    m_regServer->stopServer();
    delete m_regServer;
    m_regServer = nullptr;
  }
  if (m_nodeServer != nullptr)
  {
    m_nodeServer->stopServer();
    delete m_nodeServer;
    m_nodeServer = nullptr;
  }
  if (m_regClient != nullptr)
  {
    delete m_regClient;
    m_regClient = nullptr;
  }
}

void TestRegistryClient::testCreateBuffer()
{
  string name = "buffer1";
  BufferHandle *buffHandle = m_regClient->createBuffer(name, 1, 1024, 800);

  CPPUNIT_ASSERT(buffHandle != nullptr);
  CPPUNIT_ASSERT_EQUAL(buffHandle->name, name);

  CPPUNIT_ASSERT_EQUAL(1, ((int)buffHandle->segments.size()));
  BufferSegment segment = buffHandle->segments[0];
  CPPUNIT_ASSERT_EQUAL(1024, ((int)segment.size));
  CPPUNIT_ASSERT_EQUAL(800, ((int)segment.threshold));
}

void TestRegistryClient::testRetrieveBuffer()
{
  string name = "buffer2";
  BufferHandle *buffHandle = m_regClient->createBuffer(name, 1, 1024, 800);
  CPPUNIT_ASSERT(buffHandle != nullptr);

  buffHandle = m_regClient->retrieveBuffer(name);
  CPPUNIT_ASSERT(buffHandle != nullptr);
  CPPUNIT_ASSERT_EQUAL(buffHandle->name, name);

  CPPUNIT_ASSERT_EQUAL(1, ((int)buffHandle->segments.size()));
  BufferSegment segment = buffHandle->segments[0];
  CPPUNIT_ASSERT_EQUAL(1024, ((int)segment.size));
  CPPUNIT_ASSERT_EQUAL(800, ((int)segment.threshold));
};

void TestRegistryClient::testRegisterBuffer()
{
  string name = "buffer1";
  BufferHandle *buffHandle = new BufferHandle(name,1);
  BufferSegment segment(2000, 1024, 800);
  buffHandle->segments.push_back(segment);
  CPPUNIT_ASSERT( m_regClient->registerBuffer(buffHandle));

  BufferHandle* handle_ret = m_regClient->retrieveBuffer(name);
  CPPUNIT_ASSERT(handle_ret != nullptr);
  CPPUNIT_ASSERT_EQUAL(handle_ret->name, name);

  CPPUNIT_ASSERT_EQUAL(1, ((int)handle_ret->segments.size()));
};

void TestRegistryClient::testAppendSegment(){

  string name = "buffer2";
  BufferHandle *buffHandle = m_regClient->createBuffer(name, 1, 1024, 800);
  CPPUNIT_ASSERT(buffHandle != nullptr);

  BufferSegment segment(2000, 1024, 800);
  bool success = m_regClient->appendSegment(name, segment);
  CPPUNIT_ASSERT(success);
  buffHandle = m_regClient->retrieveBuffer(name);
  CPPUNIT_ASSERT(buffHandle != nullptr);
  CPPUNIT_ASSERT_EQUAL(buffHandle->name, name);

  CPPUNIT_ASSERT_EQUAL(2, ((int)buffHandle->segments.size()));

  BufferSegment segment1 = buffHandle->segments[0];
  CPPUNIT_ASSERT_EQUAL(1024, ((int)segment1.size));
  CPPUNIT_ASSERT_EQUAL(800, ((int)segment1.threshold));

  BufferSegment segment2 = buffHandle->segments[1];
  CPPUNIT_ASSERT_EQUAL(2000, ((int)segment2.offset));
  CPPUNIT_ASSERT_EQUAL(1024, ((int)segment2.size));
  CPPUNIT_ASSERT_EQUAL(800, ((int)segment2.threshold));
};
#pragma once

#include "../../utils/Config.h"
#include "../../dpi/flow/FlowSource.h"
#include "../../dpi/flow/FlowTarget.h"
#include "../../dpi/flow/FlowRegistryClient.h"
#include "../../dpi/flow/FlowRegistryServer.h"

struct TestTuple
{
    int64_t key;
    int64_t value;
    TestTuple(){};
    TestTuple(int64_t key, int64_t value) : key(key), value(value){};
};

class TestFlowSource : public CppUnit::TestFixture
{

  DPI_UNIT_TEST_SUITE(TestFlowSource);
    DPI_UNIT_TEST(simpleInjectConsumeTest);
    DPI_UNIT_TEST(interleavedInjectConsumeTest);
  DPI_UNIT_TEST_SUITE_END();
public:
  void setUp();
  void tearDown();
  void simpleInjectConsumeTest();
  void interleavedInjectConsumeTest();

private:

  FlowRegistryClient *m_regClient = nullptr;
  FlowRegistryServer *m_regServer = nullptr;
  FlowTarget<TestTuple> *m_flowTarget = nullptr;
  string flowName = "TestFlow";
};

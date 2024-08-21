#include <gtest/gtest.h>

#include "sc-memory/sc_event.hpp"
#include "sc-memory/sc_event_subscription.hpp"

#include "sc-memory/sc_memory.hpp"
#include "sc-memory/sc_timer.hpp"

#include "event_test_utils.hpp"

#include <atomic>
#include <thread>
#include <mutex>

TEST(ScEventQueueTest, EventsQueueDestroy)
{
  sc_memory_params params;
  sc_memory_params_clear(&params);
  params.clear = SC_TRUE;
  params.repo_path = "repo";
  params.log_level = "Debug";

  ScMemory::Initialize(params);

  ScAgentContext ctx;

  ScAddr const node = ctx.CreateNode(ScType::NodeConst);
  ScAddr const node2 = ctx.CreateNode(ScType::NodeConst);
  ScAddr const node3 = ctx.CreateNode(ScType::NodeConst);

  size_t count = 10;

  auto eventSubscription =
      ctx.CreateElementaryEventSubscription<ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm>>(
          node,
          [node, node2, count](ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm> const &)
          {
            bool result = false;
            ScMemoryContext localCtx;
            ScIterator3Ptr it = localCtx.Iterator3(node, ScType::EdgeAccessConstPosPerm, ScType::Unknown);
            while (it->Next())
              result = true;

            EXPECT_TRUE(result);
            for (size_t i = 0; i < count; ++i)
              localCtx.CreateEdge(ScType::EdgeAccessConstPosPerm, node2, node);
          });

  eventSubscription = ctx.CreateElementaryEventSubscription<ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm>>(
      node2,
      [node3, node2, count](ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm> const &)
      {
        bool result = false;
        ScMemoryContext localCtx;
        ScIterator3Ptr it = localCtx.Iterator3(node2, ScType::EdgeAccessConstPosPerm, ScType::Unknown);
        while (it->Next())
          result = true;

        EXPECT_TRUE(result);
        for (size_t i = 0; i < count; ++i)
          localCtx.CreateEdge(ScType::EdgeAccessConstPosPerm, node3, node2);
      });

  eventSubscription = ctx.CreateElementaryEventSubscription<ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm>>(
      node3,
      [node3](ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm> const &)
      {
        bool result = false;
        ScMemoryContext localCtx;
        ScIterator3Ptr it = localCtx.Iterator3(node3, ScType::EdgeAccessConstPosPerm, ScType::Unknown);
        while (it->Next())
          result = true;

        EXPECT_TRUE(result);
      });

  for (size_t i = 0; i < count; ++i)
    ctx.CreateEdge(ScType::EdgeAccessConstPosPerm, node, node2);

  sleep(5);

  ctx.Destroy();
  ScMemory::Shutdown();
}

double const kTestTimeout = 0.1;

template <ScType const & subscriptionConnectorType, ScType const & eventConnectorType>
bool TestEventSubscriptionGenerateConnector(ScAgentContext * ctx)
{
  ScAddr nodeAddr1;
  auto const & CreateNode = [&]()
  {
    nodeAddr1 = ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr1.IsValid());
  };

  ScAddr nodeAddr2;
  ScAddr arcAddr;
  auto const & EmitEvent = [&]()
  {
    nodeAddr2 = ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr2.IsValid());

    arcAddr = ctx->CreateEdge(eventConnectorType, nodeAddr2, nodeAddr1);
    EXPECT_TRUE(arcAddr.IsValid());
  };

  bool isDone = false;
  auto const & OnEvent = [&](ScEventGenerateConnector<subscriptionConnectorType> const & event)
  {
    EXPECT_EQ(event.GetGeneratedConnector(), arcAddr);
    EXPECT_EQ(event.GetGeneratedConnectorType(), eventConnectorType);
    auto [elementAddr1, elementAddr2] = event.GetConnectorIncidentElements();
    EXPECT_TRUE(elementAddr1 == nodeAddr1 || elementAddr2 == nodeAddr1);
    EXPECT_TRUE(elementAddr1 == nodeAddr2 || elementAddr2 == nodeAddr2);
    EXPECT_EQ(event.GetSubscriptionElement(), nodeAddr1);

    isDone = true;
  };

  CreateNode();

  auto eventSubscription =
      ctx->CreateElementaryEventSubscription<ScEventGenerateConnector<subscriptionConnectorType>>(nodeAddr1, OnEvent);
  ScTimer timer(kTestTimeout);

  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  return isDone;
}

TEST_F(ScEventTest, CreateEventSubscriptionGenerateConnectorAndInitiateEvent)
{
  EXPECT_TRUE((
      TestEventSubscriptionGenerateConnector<ScType::EdgeAccessConstPosPerm, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));
  EXPECT_TRUE((TestEventSubscriptionGenerateConnector<ScType::EdgeDCommonConst, ScType::EdgeDCommonConst>(&*m_ctx)));

  EXPECT_TRUE((TestEventSubscriptionGenerateConnector<ScType::EdgeAccess, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));
  EXPECT_TRUE((TestEventSubscriptionGenerateConnector<ScType::EdgeDCommon, ScType::EdgeDCommonConst>(&*m_ctx)));

  EXPECT_FALSE(
      (TestEventSubscriptionGenerateConnector<ScType::EdgeAccessConstPosPerm, ScType::EdgeDCommonConst>(&*m_ctx)));
  EXPECT_FALSE(
      (TestEventSubscriptionGenerateConnector<ScType::EdgeDCommonConst, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));

  EXPECT_FALSE((TestEventSubscriptionGenerateConnector<ScType::EdgeAccessConstPosPerm, ScType::EdgeAccess>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionGenerateConnector<ScType::EdgeDCommonConst, ScType::EdgeDCommon>(&*m_ctx)));

  EXPECT_TRUE((TestEventSubscriptionGenerateConnector<ScType::EdgeUCommonConst, ScType::EdgeUCommonConst>(&*m_ctx)));
  EXPECT_TRUE((TestEventSubscriptionGenerateConnector<ScType::EdgeUCommon, ScType::EdgeUCommonConst>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionGenerateConnector<ScType::EdgeUCommonConst, ScType::EdgeUCommon>(&*m_ctx)));
}

template <ScType const & subscriptionArcType, ScType const & eventArcType>
bool TestEventSubscriptionGenerateIncomingArc(ScAgentContext * ctx)
{
  ScAddr nodeAddr1;
  auto const & CreateNode = [&]()
  {
    nodeAddr1 = ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr1.IsValid());
  };

  ScAddr nodeAddr2;
  ScAddr arcAddr;
  auto const & EmitEvent = [&]()
  {
    nodeAddr2 = ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr2.IsValid());

    arcAddr = ctx->CreateEdge(eventArcType, nodeAddr2, nodeAddr1);
    EXPECT_TRUE(arcAddr.IsValid());
  };

  bool isDone = false;
  auto const & OnEvent = [&](ScEventGenerateIncomingArc<subscriptionArcType> const & event)
  {
    EXPECT_EQ(event.GetGeneratedArc(), arcAddr);
    EXPECT_EQ(event.GetGeneratedArcType(), eventArcType);
    EXPECT_EQ(event.GetArcSourceElement(), nodeAddr2);
    EXPECT_EQ(event.GetArcTargetElement(), nodeAddr1);
    EXPECT_EQ(event.GetSubscriptionElement(), nodeAddr1);

    isDone = true;
  };

  CreateNode();

  auto eventSubscription =
      ctx->CreateElementaryEventSubscription<ScEventGenerateIncomingArc<subscriptionArcType>>(nodeAddr1, OnEvent);
  ScTimer timer(kTestTimeout);

  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  return isDone;
}

TEST_F(ScEventTest, CreateEventSubscriptionGenerateIncomingArcAndInitiateEvent)
{
  EXPECT_TRUE((TestEventSubscriptionGenerateIncomingArc<ScType::EdgeAccessConstPosPerm, ScType::EdgeAccessConstPosPerm>(
      &*m_ctx)));
  EXPECT_TRUE((TestEventSubscriptionGenerateIncomingArc<ScType::EdgeDCommonConst, ScType::EdgeDCommonConst>(&*m_ctx)));

  EXPECT_TRUE((TestEventSubscriptionGenerateIncomingArc<ScType::EdgeAccess, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));
  EXPECT_TRUE((TestEventSubscriptionGenerateIncomingArc<ScType::EdgeDCommon, ScType::EdgeDCommonConst>(&*m_ctx)));

  EXPECT_FALSE(
      (TestEventSubscriptionGenerateIncomingArc<ScType::EdgeAccessConstPosPerm, ScType::EdgeDCommonConst>(&*m_ctx)));
  EXPECT_FALSE(
      (TestEventSubscriptionGenerateIncomingArc<ScType::EdgeDCommonConst, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));

  EXPECT_FALSE((TestEventSubscriptionGenerateIncomingArc<ScType::EdgeAccessConstPosPerm, ScType::EdgeAccess>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionGenerateIncomingArc<ScType::EdgeDCommonConst, ScType::EdgeDCommon>(&*m_ctx)));

  EXPECT_FALSE((TestEventSubscriptionGenerateIncomingArc<ScType::EdgeUCommonConst, ScType::EdgeUCommonConst>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionGenerateIncomingArc<ScType::EdgeUCommon, ScType::EdgeUCommonConst>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionGenerateIncomingArc<ScType::EdgeUCommonConst, ScType::EdgeUCommon>(&*m_ctx)));
}

template <ScType const & subscriptionArcType, ScType const & eventArcType>
bool TestEventSubscriptionGenerateOutgoingArc(ScAgentContext * ctx)
{
  ScAddr nodeAddr1;
  auto const & CreateNode = [&]()
  {
    nodeAddr1 = ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr1.IsValid());
  };

  ScAddr nodeAddr2;
  ScAddr arcAddr;
  auto const & EmitEvent = [&]()
  {
    nodeAddr2 = ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr2.IsValid());

    arcAddr = ctx->CreateEdge(eventArcType, nodeAddr1, nodeAddr2);
    EXPECT_TRUE(arcAddr.IsValid());
  };

  bool isDone = false;
  auto const & OnEvent = [&](ScEventGenerateOutgoingArc<subscriptionArcType> const & event)
  {
    EXPECT_EQ(event.GetGeneratedArc(), arcAddr);
    EXPECT_EQ(event.GetGeneratedArcType(), eventArcType);
    EXPECT_EQ(event.GetArcSourceElement(), nodeAddr1);
    EXPECT_EQ(event.GetArcTargetElement(), nodeAddr2);
    EXPECT_EQ(event.GetSubscriptionElement(), nodeAddr1);

    isDone = true;
  };

  CreateNode();

  auto eventSubscription =
      ctx->CreateElementaryEventSubscription<ScEventGenerateOutgoingArc<subscriptionArcType>>(nodeAddr1, OnEvent);
  ScTimer timer(kTestTimeout);

  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  return isDone;
}

TEST_F(ScEventTest, CreateEventSubscriptionGenerateOutgoingArcAndInitiateEvent)
{
  EXPECT_TRUE((TestEventSubscriptionGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm, ScType::EdgeAccessConstPosPerm>(
      &*m_ctx)));
  EXPECT_TRUE((TestEventSubscriptionGenerateOutgoingArc<ScType::EdgeDCommonConst, ScType::EdgeDCommonConst>(&*m_ctx)));

  EXPECT_TRUE((TestEventSubscriptionGenerateOutgoingArc<ScType::EdgeAccess, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));
  EXPECT_TRUE((TestEventSubscriptionGenerateOutgoingArc<ScType::EdgeDCommon, ScType::EdgeDCommonConst>(&*m_ctx)));

  EXPECT_FALSE(
      (TestEventSubscriptionGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm, ScType::EdgeDCommonConst>(&*m_ctx)));
  EXPECT_FALSE(
      (TestEventSubscriptionGenerateOutgoingArc<ScType::EdgeDCommonConst, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));

  EXPECT_FALSE((TestEventSubscriptionGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm, ScType::EdgeAccess>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionGenerateOutgoingArc<ScType::EdgeDCommonConst, ScType::EdgeDCommon>(&*m_ctx)));

  EXPECT_FALSE((TestEventSubscriptionGenerateOutgoingArc<ScType::EdgeUCommonConst, ScType::EdgeUCommonConst>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionGenerateOutgoingArc<ScType::EdgeUCommon, ScType::EdgeUCommonConst>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionGenerateOutgoingArc<ScType::EdgeUCommonConst, ScType::EdgeUCommon>(&*m_ctx)));
}

template <ScType const & subscriptionEdgeType, ScType const & eventEdgeType>
bool TestEventSubscriptionGenerateEdge(ScAgentContext * ctx)
{
  ScAddr nodeAddr1;
  auto const & CreateNode = [&]()
  {
    nodeAddr1 = ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr1.IsValid());
  };

  ScAddr nodeAddr2;
  ScAddr edgeAddr;
  auto const & EmitEvent = [&]()
  {
    nodeAddr2 = ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr2.IsValid());

    edgeAddr = ctx->CreateEdge(eventEdgeType, nodeAddr1, nodeAddr2);
    EXPECT_TRUE(edgeAddr.IsValid());
  };

  bool isDone = false;
  auto const & OnEvent = [&](ScEventGenerateEdge<subscriptionEdgeType> const & event)
  {
    EXPECT_EQ(event.GetGeneratedEdge(), edgeAddr);
    EXPECT_EQ(event.GetGeneratedEdgeType(), eventEdgeType);
    auto [elementAddr1, elementAddr2] = event.GetEdgeIncidentElements();
    EXPECT_TRUE(elementAddr1 == nodeAddr1 || elementAddr2 == nodeAddr1);
    EXPECT_TRUE(elementAddr1 == nodeAddr2 || elementAddr2 == nodeAddr2);
    EXPECT_EQ(event.GetSubscriptionElement(), nodeAddr1);

    isDone = true;
  };

  CreateNode();

  auto eventSubscription =
      ctx->CreateElementaryEventSubscription<ScEventGenerateEdge<subscriptionEdgeType>>(nodeAddr1, OnEvent);
  ScTimer timer(kTestTimeout);

  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  return isDone;
}

TEST_F(ScEventTest, CreateEventSubscriptionGenerateEdgeAndInitiateEvent)
{
  EXPECT_FALSE(
      (TestEventSubscriptionGenerateEdge<ScType::EdgeAccessConstPosPerm, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionGenerateEdge<ScType::EdgeDCommonConst, ScType::EdgeDCommonConst>(&*m_ctx)));

  EXPECT_FALSE((TestEventSubscriptionGenerateEdge<ScType::EdgeAccess, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionGenerateEdge<ScType::EdgeDCommon, ScType::EdgeDCommonConst>(&*m_ctx)));

  EXPECT_FALSE((TestEventSubscriptionGenerateEdge<ScType::EdgeAccessConstPosPerm, ScType::EdgeDCommonConst>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionGenerateEdge<ScType::EdgeDCommonConst, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));

  EXPECT_FALSE((TestEventSubscriptionGenerateEdge<ScType::EdgeAccessConstPosPerm, ScType::EdgeAccess>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionGenerateEdge<ScType::EdgeDCommonConst, ScType::EdgeDCommon>(&*m_ctx)));

  EXPECT_TRUE((TestEventSubscriptionGenerateEdge<ScType::EdgeUCommonConst, ScType::EdgeUCommonConst>(&*m_ctx)));
  EXPECT_TRUE((TestEventSubscriptionGenerateEdge<ScType::EdgeUCommon, ScType::EdgeUCommonConst>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionGenerateEdge<ScType::EdgeUCommonConst, ScType::EdgeUCommon>(&*m_ctx)));
}

template <ScType const & subscriptionConnectorType, ScType const & eventConnectorType>
bool TestEventSubscriptionEraseConnector(ScAgentContext * ctx)
{
  ScAddr nodeAddr1;
  auto const & CreateNode = [&]()
  {
    nodeAddr1 = ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr1.IsValid());
  };

  ScAddr nodeAddr2;
  ScAddr arcAddr;
  auto const & EmitEvent = [&]()
  {
    nodeAddr2 = ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr2.IsValid());

    arcAddr = ctx->CreateEdge(eventConnectorType, nodeAddr2, nodeAddr1);
    EXPECT_TRUE(arcAddr.IsValid());

    ctx->EraseElement(arcAddr);
  };

  bool isDone = false;
  auto const & OnEvent = [&](ScEventEraseConnector<subscriptionConnectorType> const & event)
  {
    EXPECT_EQ(event.GetErasableConnector(), arcAddr);
    EXPECT_TRUE(ctx->IsElement(arcAddr));
    EXPECT_EQ(event.GetErasableConnectorType(), eventConnectorType);
    auto [elementAddr1, elementAddr2] = event.GetConnectorIncidentElements();
    EXPECT_TRUE(elementAddr1 == nodeAddr1 || elementAddr2 == nodeAddr1);
    EXPECT_TRUE(elementAddr1 == nodeAddr2 || elementAddr2 == nodeAddr2);
    EXPECT_EQ(event.GetSubscriptionElement(), nodeAddr1);

    isDone = true;
  };

  CreateNode();

  auto eventSubscription =
      ctx->CreateElementaryEventSubscription<ScEventEraseConnector<subscriptionConnectorType>>(nodeAddr1, OnEvent);
  ScTimer timer(kTestTimeout);

  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  return isDone;
}

TEST_F(ScEventTest, CreateEventSubscriptionEraseConnectorAndInitiateEvent)
{
  EXPECT_TRUE(
      (TestEventSubscriptionEraseConnector<ScType::EdgeAccessConstPosPerm, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));
  EXPECT_TRUE((TestEventSubscriptionEraseConnector<ScType::EdgeDCommonConst, ScType::EdgeDCommonConst>(&*m_ctx)));

  EXPECT_TRUE((TestEventSubscriptionEraseConnector<ScType::EdgeAccess, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));
  EXPECT_TRUE((TestEventSubscriptionEraseConnector<ScType::EdgeDCommon, ScType::EdgeDCommonConst>(&*m_ctx)));

  EXPECT_FALSE(
      (TestEventSubscriptionEraseConnector<ScType::EdgeAccessConstPosPerm, ScType::EdgeDCommonConst>(&*m_ctx)));
  EXPECT_FALSE(
      (TestEventSubscriptionEraseConnector<ScType::EdgeDCommonConst, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));

  EXPECT_FALSE((TestEventSubscriptionEraseConnector<ScType::EdgeAccessConstPosPerm, ScType::EdgeAccess>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionEraseConnector<ScType::EdgeDCommonConst, ScType::EdgeDCommon>(&*m_ctx)));

  EXPECT_TRUE((TestEventSubscriptionEraseConnector<ScType::EdgeUCommonConst, ScType::EdgeUCommonConst>(&*m_ctx)));
  EXPECT_TRUE((TestEventSubscriptionEraseConnector<ScType::EdgeUCommon, ScType::EdgeUCommonConst>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionEraseConnector<ScType::EdgeUCommonConst, ScType::EdgeUCommon>(&*m_ctx)));
}

template <ScType const & subscriptionArcType, ScType const & eventArcType>
bool TestEventSubscriptionEraseIncomingArc(ScAgentContext * ctx)
{
  ScAddr nodeAddr1;
  auto const & CreateNode = [&]()
  {
    nodeAddr1 = ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr1.IsValid());
  };

  ScAddr nodeAddr2;
  ScAddr arcAddr;
  auto const & EmitEvent = [&]()
  {
    nodeAddr2 = ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr2.IsValid());

    arcAddr = ctx->CreateEdge(eventArcType, nodeAddr2, nodeAddr1);
    EXPECT_TRUE(arcAddr.IsValid());

    ctx->EraseElement(arcAddr);
  };

  bool isDone = false;
  auto const & OnEvent = [&](ScEventEraseIncomingArc<subscriptionArcType> const & event)
  {
    EXPECT_EQ(event.GetErasableArc(), arcAddr);
    EXPECT_TRUE(ctx->IsElement(arcAddr));
    EXPECT_EQ(event.GetErasableArcType(), eventArcType);
    EXPECT_EQ(event.GetArcSourceElement(), nodeAddr2);
    EXPECT_EQ(event.GetArcTargetElement(), nodeAddr1);
    EXPECT_EQ(event.GetSubscriptionElement(), nodeAddr1);

    isDone = true;
  };

  CreateNode();

  auto eventSubscription =
      ctx->CreateElementaryEventSubscription<ScEventEraseIncomingArc<subscriptionArcType>>(nodeAddr1, OnEvent);
  ScTimer timer(kTestTimeout);

  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  return isDone;
}

TEST_F(ScEventTest, CreateEventSubscriptionEraseIncomingArcAndInitiateEvent)
{
  EXPECT_TRUE(
      (TestEventSubscriptionEraseIncomingArc<ScType::EdgeAccessConstPosPerm, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));
  EXPECT_TRUE((TestEventSubscriptionEraseIncomingArc<ScType::EdgeDCommonConst, ScType::EdgeDCommonConst>(&*m_ctx)));

  EXPECT_TRUE((TestEventSubscriptionEraseIncomingArc<ScType::EdgeAccess, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));
  EXPECT_TRUE((TestEventSubscriptionEraseIncomingArc<ScType::EdgeDCommon, ScType::EdgeDCommonConst>(&*m_ctx)));

  EXPECT_FALSE(
      (TestEventSubscriptionEraseIncomingArc<ScType::EdgeAccessConstPosPerm, ScType::EdgeDCommonConst>(&*m_ctx)));
  EXPECT_FALSE(
      (TestEventSubscriptionEraseIncomingArc<ScType::EdgeDCommonConst, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));

  EXPECT_FALSE((TestEventSubscriptionEraseIncomingArc<ScType::EdgeAccessConstPosPerm, ScType::EdgeAccess>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionEraseIncomingArc<ScType::EdgeDCommonConst, ScType::EdgeDCommon>(&*m_ctx)));

  EXPECT_FALSE((TestEventSubscriptionEraseIncomingArc<ScType::EdgeUCommonConst, ScType::EdgeUCommonConst>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionEraseIncomingArc<ScType::EdgeUCommon, ScType::EdgeUCommonConst>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionEraseIncomingArc<ScType::EdgeUCommonConst, ScType::EdgeUCommon>(&*m_ctx)));
}

template <ScType const & subscriptionArcType, ScType const & eventArcType>
bool TestEventSubscriptionEraseOutgoingArc(ScAgentContext * ctx)
{
  ScAddr nodeAddr1;
  auto const & CreateNode = [&]()
  {
    nodeAddr1 = ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr1.IsValid());
  };

  ScAddr nodeAddr2;
  ScAddr arcAddr;
  auto const & EmitEvent = [&]()
  {
    nodeAddr2 = ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr2.IsValid());

    arcAddr = ctx->CreateEdge(eventArcType, nodeAddr1, nodeAddr2);
    EXPECT_TRUE(arcAddr.IsValid());

    ctx->EraseElement(arcAddr);
  };

  bool isDone = false;
  auto const & OnEvent = [&](ScEventEraseOutgoingArc<subscriptionArcType> const & event)
  {
    EXPECT_EQ(event.GetErasableArc(), arcAddr);
    EXPECT_TRUE(ctx->IsElement(arcAddr));
    EXPECT_EQ(event.GetErasableArcType(), eventArcType);
    EXPECT_EQ(event.GetArcSourceElement(), nodeAddr1);
    EXPECT_EQ(event.GetArcTargetElement(), nodeAddr2);
    EXPECT_EQ(event.GetSubscriptionElement(), nodeAddr1);

    isDone = true;
  };

  CreateNode();

  auto eventSubscription =
      ctx->CreateElementaryEventSubscription<ScEventEraseOutgoingArc<subscriptionArcType>>(nodeAddr1, OnEvent);
  ScTimer timer(kTestTimeout);

  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  return isDone;
}

TEST_F(ScEventTest, CreateEventSubscriptionEraseOutgoingArcAndInitiateEvent)
{
  EXPECT_TRUE(
      (TestEventSubscriptionEraseOutgoingArc<ScType::EdgeAccessConstPosPerm, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));
  EXPECT_TRUE((TestEventSubscriptionEraseOutgoingArc<ScType::EdgeDCommonConst, ScType::EdgeDCommonConst>(&*m_ctx)));

  EXPECT_TRUE((TestEventSubscriptionEraseOutgoingArc<ScType::EdgeAccess, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));
  EXPECT_TRUE((TestEventSubscriptionEraseOutgoingArc<ScType::EdgeDCommon, ScType::EdgeDCommonConst>(&*m_ctx)));

  EXPECT_FALSE(
      (TestEventSubscriptionEraseOutgoingArc<ScType::EdgeAccessConstPosPerm, ScType::EdgeDCommonConst>(&*m_ctx)));
  EXPECT_FALSE(
      (TestEventSubscriptionEraseOutgoingArc<ScType::EdgeDCommonConst, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));

  EXPECT_FALSE((TestEventSubscriptionEraseOutgoingArc<ScType::EdgeAccessConstPosPerm, ScType::EdgeAccess>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionEraseOutgoingArc<ScType::EdgeDCommonConst, ScType::EdgeDCommon>(&*m_ctx)));

  EXPECT_FALSE((TestEventSubscriptionEraseOutgoingArc<ScType::EdgeUCommonConst, ScType::EdgeUCommonConst>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionEraseOutgoingArc<ScType::EdgeUCommon, ScType::EdgeUCommonConst>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionEraseOutgoingArc<ScType::EdgeUCommonConst, ScType::EdgeUCommon>(&*m_ctx)));
}

template <ScType const & subscriptionEdgeType, ScType const & eventEdgeType>
bool TestEventSubscriptionEraseEdge(ScAgentContext * ctx)
{
  ScAddr nodeAddr1;
  auto const & CreateNode = [&]()
  {
    nodeAddr1 = ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr1.IsValid());
  };

  ScAddr nodeAddr2;
  ScAddr edgeAddr;
  auto const & EmitEvent = [&]()
  {
    nodeAddr2 = ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr2.IsValid());

    edgeAddr = ctx->CreateEdge(eventEdgeType, nodeAddr1, nodeAddr2);
    EXPECT_TRUE(edgeAddr.IsValid());

    ctx->EraseElement(edgeAddr);
  };

  bool isDone = false;
  auto const & OnEvent = [&](ScEventEraseEdge<subscriptionEdgeType> const & event)
  {
    EXPECT_EQ(event.GetErasableEdge(), edgeAddr);
    EXPECT_TRUE(ctx->IsElement(edgeAddr));
    EXPECT_EQ(event.GetErasableEdgeType(), eventEdgeType);
    auto [elementAddr1, elementAddr2] = event.GetEdgeIncidentElements();
    EXPECT_TRUE(elementAddr1 == nodeAddr1 || elementAddr2 == nodeAddr1);
    EXPECT_TRUE(elementAddr1 == nodeAddr2 || elementAddr2 == nodeAddr2);
    EXPECT_EQ(event.GetSubscriptionElement(), nodeAddr1);

    isDone = true;
  };

  CreateNode();

  auto eventSubscription =
      ctx->CreateElementaryEventSubscription<ScEventEraseEdge<subscriptionEdgeType>>(nodeAddr1, OnEvent);
  ScTimer timer(kTestTimeout);

  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  return isDone;
}

TEST_F(ScEventTest, CreateEventSubscriptionEraseEdgeAndInitiateEvent)
{
  EXPECT_FALSE(
      (TestEventSubscriptionEraseEdge<ScType::EdgeAccessConstPosPerm, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionEraseEdge<ScType::EdgeDCommonConst, ScType::EdgeDCommonConst>(&*m_ctx)));

  EXPECT_FALSE((TestEventSubscriptionEraseEdge<ScType::EdgeAccess, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionEraseEdge<ScType::EdgeDCommon, ScType::EdgeDCommonConst>(&*m_ctx)));

  EXPECT_FALSE((TestEventSubscriptionEraseEdge<ScType::EdgeAccessConstPosPerm, ScType::EdgeDCommonConst>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionEraseEdge<ScType::EdgeDCommonConst, ScType::EdgeAccessConstPosPerm>(&*m_ctx)));

  EXPECT_FALSE((TestEventSubscriptionEraseEdge<ScType::EdgeAccessConstPosPerm, ScType::EdgeAccess>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionEraseEdge<ScType::EdgeDCommonConst, ScType::EdgeDCommon>(&*m_ctx)));

  EXPECT_TRUE((TestEventSubscriptionEraseEdge<ScType::EdgeUCommonConst, ScType::EdgeUCommonConst>(&*m_ctx)));
  EXPECT_TRUE((TestEventSubscriptionEraseEdge<ScType::EdgeUCommon, ScType::EdgeUCommonConst>(&*m_ctx)));
  EXPECT_FALSE((TestEventSubscriptionEraseEdge<ScType::EdgeUCommonConst, ScType::EdgeUCommon>(&*m_ctx)));
}

TEST_F(ScEventTest, CreateEventSubscriptionEraseElementAndInitiateEvent)
{
  ScAddr nodeAddr1;
  auto const & CreateNode = [this, &nodeAddr1]()
  {
    nodeAddr1 = m_ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr1.IsValid());
  };

  auto const & EmitEvent = [this, &nodeAddr1]()
  {
    m_ctx->EraseElement(nodeAddr1);
  };

  bool isDone = false;
  auto const & OnEvent = [&](ScEventEraseElement const & event)
  {
    EXPECT_TRUE(m_ctx->IsElement(nodeAddr1));
    isDone = true;
  };

  CreateNode();

  auto eventSubscription = m_ctx->CreateElementaryEventSubscription<ScEventEraseElement>(nodeAddr1, OnEvent);
  ScTimer timer(kTestTimeout);

  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(isDone);
}

TEST_F(ScEventTest, CreateEventSubscriptionEraseElementAndInitiateEventAndGetRemovedElementType)
{
  ScAddr nodeAddr1;
  auto const & CreateNode = [this, &nodeAddr1]()
  {
    nodeAddr1 = m_ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr1.IsValid());
  };

  auto const & EmitEvent = [this, &nodeAddr1]()
  {
    m_ctx->EraseElement(nodeAddr1);
  };

  bool isDone = false;
  auto const & OnEvent = [&](ScEventEraseElement const & event)
  {
    EXPECT_EQ(event.GetSubscriptionElement(), nodeAddr1);
    EXPECT_TRUE(m_ctx->IsElement(nodeAddr1));
    EXPECT_NO_THROW(m_ctx->GetElementType(nodeAddr1));
  };

  CreateNode();

  auto eventSubscription = m_ctx->CreateElementaryEventSubscription<ScEventEraseElement>(nodeAddr1, OnEvent);
  ScTimer timer(kTestTimeout);

  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(isDone);
}

TEST_F(ScEventTest, CreateEventSubscriptionEraseElementAndInitiateEventAndThrowException)
{
  ScAddr nodeAddr1;
  auto const & CreateNode = [this, &nodeAddr1]()
  {
    nodeAddr1 = m_ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr1.IsValid());
  };

  auto const & EmitEvent = [this, &nodeAddr1]()
  {
    m_ctx->EraseElement(nodeAddr1);
  };

  bool isDone = false;
  auto const & OnEvent = [&](ScEventEraseElement const &)
  {
    SC_THROW_EXCEPTION(utils::ExceptionInvalidState, "Test exception");
  };

  CreateNode();

  auto eventSubscription = m_ctx->CreateElementaryEventSubscription<ScEventEraseElement>(nodeAddr1, OnEvent);
  ScTimer timer(kTestTimeout);

  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(isDone);
}

TEST_F(ScEventTest, CreateEventSubscriptionEraseElementAndInitiateEventAndGetRemovedElementTypeV2)
{
  ScAddr nodeAddr1;
  auto const & CreateNode = [this, &nodeAddr1]()
  {
    nodeAddr1 = m_ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr1.IsValid());
  };

  auto const & EmitEvent = [this, &nodeAddr1]()
  {
    m_ctx->EraseElement(nodeAddr1);
  };

  bool isDone = false;
  auto const & OnEvent = [&](ScElementaryEvent const & event)
  {
    EXPECT_TRUE(m_ctx->IsElement(nodeAddr1));
    m_ctx->GetElementType(nodeAddr1);
  };

  CreateNode();

  auto eventSubscription =
      m_ctx->CreateElementaryEventSubscription(ScKeynodes::sc_event_erase_element, nodeAddr1, OnEvent);
  ScTimer timer(kTestTimeout);

  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(isDone);
}

TEST_F(ScEventTest, CreateEventSubscriptionEraseElementAndInitiateEventAndThrowExceptionV2)
{
  ScAddr nodeAddr1;
  auto const & CreateNode = [this, &nodeAddr1]()
  {
    nodeAddr1 = m_ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(nodeAddr1.IsValid());
  };

  auto const & EmitEvent = [this, &nodeAddr1]()
  {
    m_ctx->EraseElement(nodeAddr1);
  };

  bool isDone = false;
  auto const & OnEvent = [&](ScElementaryEvent const &)
  {
    SC_THROW_EXCEPTION(utils::ExceptionInvalidState, "Test exception");
  };

  CreateNode();

  auto eventSubscription =
      m_ctx->CreateElementaryEventSubscription(ScKeynodes::sc_event_erase_element, nodeAddr1, OnEvent);
  ScTimer timer(kTestTimeout);

  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(isDone);
}

TEST_F(ScEventTest, CreateEventSubscriptionChangeLinkContentAndInitiateEvent)
{
  ScAddr linkAddr;
  auto const & CreateNode = [this, &linkAddr]()
  {
    linkAddr = m_ctx->CreateLink(ScType::LinkConst);
    m_ctx->SetLinkContent(linkAddr, "old content");
    EXPECT_TRUE(linkAddr.IsValid());
  };

  auto const & EmitEvent = [this, &linkAddr]()
  {
    m_ctx->SetLinkContent(linkAddr, "new content");
  };

  bool isDone = false;
  auto const & OnEvent = [&](ScEventChangeLinkContent const & event)
  {
    EXPECT_EQ(event.GetSubscriptionElement(), linkAddr);
    isDone = true;
  };

  CreateNode();

  auto eventSubscription = m_ctx->CreateElementaryEventSubscription<ScEventChangeLinkContent>(linkAddr, OnEvent);
  ScTimer timer(kTestTimeout);

  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(isDone);
}

TEST_F(ScEventTest, InvalidSubscriptions)
{
  ScAddr nodeAddr;

  EXPECT_THROW(
      m_ctx->CreateElementaryEventSubscription<ScEventGenerateConnector<ScType::EdgeAccess>>(nodeAddr, {}),
      utils::ExceptionInvalidParams);
  EXPECT_THROW(
      m_ctx->CreateElementaryEventSubscription<ScEventGenerateOutgoingArc<ScType::EdgeAccess>>(nodeAddr, {}),
      utils::ExceptionInvalidParams);
  EXPECT_THROW(
      m_ctx->CreateElementaryEventSubscription<ScEventGenerateIncomingArc<ScType::EdgeAccess>>(nodeAddr, {}),
      utils::ExceptionInvalidParams);
  EXPECT_THROW(
      m_ctx->CreateElementaryEventSubscription<ScEventGenerateEdge<ScType::EdgeAccess>>(nodeAddr, {}),
      utils::ExceptionInvalidParams);
  EXPECT_THROW(
      m_ctx->CreateElementaryEventSubscription<ScEventEraseConnector<ScType::EdgeAccess>>(nodeAddr, {}),
      utils::ExceptionInvalidParams);
  EXPECT_THROW(
      m_ctx->CreateElementaryEventSubscription<ScEventEraseOutgoingArc<ScType::EdgeAccess>>(nodeAddr, {}),
      utils::ExceptionInvalidParams);
  EXPECT_THROW(
      m_ctx->CreateElementaryEventSubscription<ScEventEraseIncomingArc<ScType::EdgeAccess>>(nodeAddr, {}),
      utils::ExceptionInvalidParams);
  EXPECT_THROW(
      m_ctx->CreateElementaryEventSubscription<ScEventEraseEdge<ScType::EdgeAccess>>(nodeAddr, {}),
      utils::ExceptionInvalidParams);
  EXPECT_THROW(
      m_ctx->CreateElementaryEventSubscription<ScEventEraseElement>(nodeAddr, {}), utils::ExceptionInvalidParams);
  EXPECT_THROW(
      m_ctx->CreateElementaryEventSubscription<ScEventChangeLinkContent>(nodeAddr, {}), utils::ExceptionInvalidParams);

  nodeAddr = m_ctx->CreateNode(ScType::NodeConst);
  EXPECT_THROW(
      m_ctx->CreateElementaryEventSubscription<ScEventChangeLinkContent>(nodeAddr, {}), utils::ExceptionInvalidParams);
}

TEST_F(ScEventTest, InvalidEvents)
{
  ScAddr nodeAddr;
  ScAddr eventClassAddr;
  EXPECT_THROW(m_ctx->CreateElementaryEventSubscription(eventClassAddr, nodeAddr, {}), utils::ExceptionInvalidParams);

  eventClassAddr = m_ctx->CreateNode(ScType::NodeConst);
  EXPECT_THROW(m_ctx->CreateElementaryEventSubscription(eventClassAddr, nodeAddr, {}), utils::ExceptionInvalidParams);

  eventClassAddr = ScKeynodes::sc_event_change_link_content;
  EXPECT_THROW(m_ctx->CreateElementaryEventSubscription(eventClassAddr, nodeAddr, {}), utils::ExceptionInvalidParams);

  nodeAddr = m_ctx->CreateNode(ScType::NodeConst);
  EXPECT_THROW(m_ctx->CreateElementaryEventSubscription(eventClassAddr, nodeAddr, {}), utils::ExceptionInvalidParams);
}

TEST_F(ScEventTest, SetRemoveDelegateFunc)
{
  ScAddr linkAddr;
  auto const & CreateNode = [this, &linkAddr]()
  {
    linkAddr = m_ctx->CreateLink(ScType::LinkConst);
    m_ctx->SetLinkContent(linkAddr, "old content");
    EXPECT_TRUE(linkAddr.IsValid());
  };

  auto const & EmitEvent = [this, &linkAddr]()
  {
    m_ctx->SetLinkContent(linkAddr, "new content");
  };

  bool isDone = false;
  auto const & OnEvent = [&](ScEventChangeLinkContent const & event)
  {
    EXPECT_EQ(event.GetSubscriptionElement(), linkAddr);
    isDone = true;
  };

  CreateNode();
  auto eventSubscription = m_ctx->CreateElementaryEventSubscription<ScEventChangeLinkContent>(linkAddr, {});

  ScTimer timer = ScTimer(kTestTimeout);
  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(isDone);

  eventSubscription->SetDelegate(OnEvent);

  timer = ScTimer(kTestTimeout);
  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(isDone);
  isDone = false;

  eventSubscription->RemoveDelegate();

  timer = ScTimer(kTestTimeout);
  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(isDone);
}

TEST_F(ScEventTest, SetRemoveDelegateFuncV2)
{
  ScAddr linkAddr;
  auto const & CreateNode = [this, &linkAddr]()
  {
    linkAddr = m_ctx->CreateLink(ScType::LinkConst);
    m_ctx->SetLinkContent(linkAddr, "old content");
    EXPECT_TRUE(linkAddr.IsValid());
  };

  auto const & EmitEvent = [this, &linkAddr]()
  {
    m_ctx->SetLinkContent(linkAddr, "new content");
  };

  bool isDone = false;
  auto const & OnEvent = [&](ScElementaryEvent const & event)
  {
    EXPECT_EQ(event.GetSubscriptionElement(), linkAddr);
    isDone = true;
  };

  CreateNode();

  auto eventSubscription =
      m_ctx->CreateElementaryEventSubscription(ScKeynodes::sc_event_change_link_content, linkAddr, {});

  ScTimer timer = ScTimer(kTestTimeout);
  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(isDone);

  eventSubscription->SetDelegate(OnEvent);

  timer = ScTimer(kTestTimeout);
  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(isDone);
  isDone = false;

  eventSubscription->RemoveDelegate();

  timer = ScTimer(kTestTimeout);
  EmitEvent();

  while (!isDone && !timer.IsTimeOut())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(isDone);
}

TEST_F(ScEventTest, DestroyOrder)
{
  ScAddr const node = m_ctx->CreateNode(ScType::Unknown);
  EXPECT_TRUE(node.IsValid());

  auto evt = m_ctx->CreateElementaryEventSubscription<ScEventGenerateOutgoingArc<ScType::EdgeAccess>>(
      node, [](ScEventGenerateOutgoingArc<ScType::EdgeAccess> const &) {});

  m_ctx.reset();

  // delete event after context
}

TEST_F(ScEventTest, EventsLock)
{
  ScAddr const node = m_ctx->CreateNode(ScType::NodeConst);
  ScAddr const node2 = m_ctx->CreateNode(ScType::NodeConst);

  auto eventSubscription =
      m_ctx->CreateElementaryEventSubscription<ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm>>(
          node,
          [node](ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm> const &)
          {
            bool result = false;
            ScMemoryContext localCtx;
            ScIterator3Ptr it = localCtx.Iterator3(node, ScType::EdgeAccessConstPosPerm, ScType::Unknown);
            while (it->Next())
              result = true;

            EXPECT_TRUE(result);
          });

  for (size_t i = 0; i < 10000; i++)
    m_ctx->CreateEdge(ScType::EdgeAccessConstPosPerm, node, node2);
}

TEST_F(ScEventTest, ParallelCreateEdges)
{
  ScAddr const node = m_ctx->CreateNode(ScType::NodeConst);
  ScAddr const node2 = m_ctx->CreateNode(ScType::NodeConst);

  auto eventSubscription =
      m_ctx->CreateElementaryEventSubscription<ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm>>(
          node,
          [node](ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm> const & event)
          {
            bool result = false;
            ScMemoryContext localCtx;
            ScIterator3Ptr it = localCtx.Iterator3(node, ScType::EdgeAccessConstPosPerm, ScType::Unknown);
            while (it->Next())
              result = true;

            for (size_t i = 0; i < 10000; i++)
              localCtx.CreateEdge(ScType::EdgeAccessConstPosPerm, node, event.GetArcTargetElement());

            EXPECT_TRUE(result);
          });

  for (size_t i = 0; i < 10000; i++)
    m_ctx->CreateEdge(ScType::EdgeAccessConstPosPerm, node, node2);
}

TEST_F(ScEventTest, ParallelCreateEraseEdges)
{
  ScAddr const node = m_ctx->CreateNode(ScType::NodeConst);
  ScAddr const node2 = m_ctx->CreateNode(ScType::NodeConst);

  auto eventSubscription =
      m_ctx->CreateElementaryEventSubscription<ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm>>(
          node,
          [node](ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm> const &)
          {
            ScMemoryContext localCtx;
            ScIterator3Ptr it = localCtx.Iterator3(node, ScType::EdgeAccessConstPosPerm, ScType::Unknown);
            while (it->Next())
              localCtx.EraseElement(it->Get(1));
          });

  for (size_t i = 0; i < 10000; i++)
    m_ctx->CreateEdge(ScType::EdgeAccessConstPosPerm, node, node2);
}

TEST_F(ScEventTest, ParallelCreateEraseEdges2)
{
  ScAddr const node = m_ctx->CreateNode(ScType::NodeConst);
  ScAddr const node2 = m_ctx->CreateNode(ScType::NodeConst);

  auto eventSubscription =
      m_ctx->CreateElementaryEventSubscription<ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm>>(
          node,
          [](ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm> const & event)
          {
            ScMemoryContext localCtx;
            localCtx.EraseElement(event.GetGeneratedArc());
          });

  for (size_t i = 0; i < 10000; i++)
    m_ctx->CreateEdge(ScType::EdgeAccessConstPosPerm, node, node2);
}

TEST_F(ScEventTest, PendEvents)
{
  /* Main idea of test: create two sets with N elements, and add edges to relations.
   * Every time, when event emits, we should check, that whole construction exist
   */
  ScAddr const set1 = m_ctx->CreateNode(ScType::NodeConstClass);
  ScAddr const rel = m_ctx->CreateNode(ScType::NodeConstNoRole);

  static size_t const el_num = 1 << 10;
  std::vector<ScAddr> elements(el_num);
  for (size_t i = 0; i < el_num; ++i)
  {
    ScAddr const a = m_ctx->CreateNode(ScType::NodeConst);
    EXPECT_TRUE(a.IsValid());
    elements[i] = a;
  }

  // create template for pending events check
  ScTemplate templ;
  for (auto const & a : elements)
  {
    templ.Quintuple(set1, ScType::EdgeDCommonVar, a >> "_el", ScType::EdgeAccessVarPosPerm, rel);
  }

  std::atomic_uint eventsCount(0);
  std::atomic_uint passedCount(0);

  auto eventSubscription =
      m_ctx->CreateElementaryEventSubscription<ScEventGenerateOutgoingArc<ScType::EdgeDCommonConst>>(
          set1,
          [&passedCount, &eventsCount, &set1, &elements, &rel](
              ScEventGenerateOutgoingArc<ScType::EdgeDCommonConst> const &)
          {
            std::shared_ptr<ScTemplate> checkTempl(new ScTemplate());
            size_t step = 100;
            size_t testNum = el_num / step - 1;
            for (size_t i = 0; i < testNum; ++i)
            {
              checkTempl->Quintuple(
                  set1, ScType::EdgeDCommonVar, elements[i * step] >> "_el", ScType::EdgeAccessVarPosPerm, rel);
            }

            ScMemoryContext localCtx;
            EXPECT_TRUE(localCtx.IsValid());

            ScTemplateSearchResult res;
            EXPECT_TRUE(localCtx.HelperSearchTemplate(*checkTempl, res));

            if (res.Size() == 1)
              passedCount.fetch_add(1);

            eventsCount.fetch_add(1);
          });

  ScTemplateGenResult genResult;
  EXPECT_TRUE(m_ctx->HelperGenTemplate(templ, genResult));

  // wait all events
  while (eventsCount.load() < el_num)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

  EXPECT_EQ(passedCount, el_num);
}

TEST_F(ScEventTest, BlockEventsAndNotEmitAfter)
{
  ScAddr const nodeAddr = m_ctx->CreateNode(ScType::NodeConst);

  std::atomic_bool isCalled = false;
  auto eventSubscription =
      m_ctx->CreateElementaryEventSubscription<ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm>>(
          nodeAddr,
          [&isCalled](ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm> const &)
          {
            isCalled = true;
          });

  m_ctx->BeingEventsBlocking();

  ScAddr const nodeAddr2 = m_ctx->CreateNode(ScType::NodeConst);
  m_ctx->CreateEdge(ScType::EdgeAccessConstPosPerm, nodeAddr, nodeAddr2);

  m_ctx->EndEventsBlocking();

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_FALSE(isCalled);
}

TEST_F(ScEventTest, BlockEventsAndEmitAfter)
{
  ScAddr const nodeAddr = m_ctx->CreateNode(ScType::NodeConst);

  std::atomic_bool isCalled = false;
  auto eventSubscription =
      m_ctx->CreateElementaryEventSubscription<ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm>>(
          nodeAddr,
          [&isCalled](ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm> const &)
          {
            isCalled = true;
          });

  m_ctx->BeingEventsBlocking();

  ScAddr nodeAddr2 = m_ctx->CreateNode(ScType::NodeConst);
  m_ctx->CreateEdge(ScType::EdgeAccessConstPosPerm, nodeAddr, nodeAddr2);

  m_ctx->EndEventsBlocking();

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_FALSE(isCalled);

  nodeAddr2 = m_ctx->CreateNode(ScType::NodeConst);
  m_ctx->CreateEdge(ScType::EdgeAccessConstPosPerm, nodeAddr, nodeAddr2);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_TRUE(isCalled);
}

TEST_F(ScEventTest, BlockEventsGuardAndNotEmitAfter)
{
  ScAddr const nodeAddr = m_ctx->CreateNode(ScType::NodeConst);

  std::atomic_bool isCalled = false;
  auto eventSubscription =
      m_ctx->CreateElementaryEventSubscription<ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm>>(
          nodeAddr,
          [&isCalled](ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm> const &)
          {
            isCalled = true;
          });

  ScMemoryContextEventsBlockingGuard guard(*m_ctx);

  ScAddr const nodeAddr2 = m_ctx->CreateNode(ScType::NodeConst);
  m_ctx->CreateEdge(ScType::EdgeAccessConstPosPerm, nodeAddr, nodeAddr2);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_FALSE(isCalled);
}

TEST_F(ScEventTest, BlockEventsGuardAndEmitAfter)
{
  ScAddr const nodeAddr = m_ctx->CreateNode(ScType::NodeConst);

  std::atomic_bool isCalled = false;
  auto eventSubscription =
      m_ctx->CreateElementaryEventSubscription<ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm>>(
          nodeAddr,
          [&isCalled](ScEventGenerateOutgoingArc<ScType::EdgeAccessConstPosPerm> const &)
          {
            isCalled = true;
          });
  {
    ScMemoryContextEventsBlockingGuard guard(*m_ctx);

    ScAddr const nodeAddr2 = m_ctx->CreateNode(ScType::NodeConst);
    m_ctx->CreateEdge(ScType::EdgeAccessConstPosPerm, nodeAddr, nodeAddr2);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_FALSE(isCalled);
  }

  ScAddr const nodeAddr2 = m_ctx->CreateNode(ScType::NodeConst);
  m_ctx->CreateEdge(ScType::EdgeAccessConstPosPerm, nodeAddr, nodeAddr2);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_TRUE(isCalled);
}

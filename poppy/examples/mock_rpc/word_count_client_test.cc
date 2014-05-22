// Copyright 2011, Tencent Inc.
// Author: Yi Wang <yiwang@tencent.com>
//
// This unittest uses the service mock generation feature of protoc
// and gmock to test the log store client facility.
//
// For more information on how we modify Protobuf's compiler, protoc,
// to generate gmock-compatible mock service stub, please refer to
// files in thirdparty/protobuf-2.3.0/protobuf/compiler/cpp.
//
#include <string>
#include "common/base/module.h"
#include "common/base/scoped_ptr.h"
#include "common/system/concurrency/event.h"
#include "poppy/examples/mock_rpc/word_count_client.h"
#include "poppy/rpc_mock.h"
#include "thirdparty/gtest/gtest.h"
#include "thirdparty/protobuf/service.h"

namespace poppy {
namespace examples {
namespace mock_rpc {

class MockRPCTest : public testing::Test
{
public:
    MockRPCTest() : m_count(0)
    {
        m_counter.reset(new WordCounter("/mock/wordcount"));
    }

    void SyncCallCheck(std::string text, int expect_count)
    {
        m_counter->Count(text, &m_count, NULL);
        EXPECT_EQ(expect_count, m_count);
    }

    void AsyncCallCheck(std::string text, int expect_count)
    {
        ::google::protobuf::Closure* callback =
            NewClosure(this, &MockRPCTest::CountCallback, expect_count);
        m_counter->Count(text, &m_count, callback);

        // wait for callback run
        m_callback_event.Wait();
    }

private:
    void CountCallback(int expect_count)
    {
        EXPECT_EQ(expect_count, m_count);
        m_callback_event.Set();
    }

protected:
    scoped_ptr<WordCounter> m_counter;
    int                     m_count;
    WordCountRequest        m_request;
    WordCountResponse       m_response;
    AutoResetEvent          m_callback_event;
};

TEST_F(MockRPCTest, ProtoMessageEqRespond) {
    // test for sync rpc_mock
    m_request.set_text("Steve Jobs");
    m_response.set_count(1);
    EXPECT_RPC("poppy.examples.mock_rpc.WordCountService.Count",
               poppy::ProtoMessageEq(m_request))
            .WillOnce(poppy::Respond(m_response));
    SyncCallCheck("Steve Jobs", 1);

    // test for async rpc_mock
    EXPECT_RPC("poppy.examples.mock_rpc.WordCountService.Count",
               poppy::ProtoMessageEq(m_request))
            .WillOnce(poppy::Respond(m_response));
    AsyncCallCheck("Steve Jobs", 1);
}

TEST_F(MockRPCTest, SingularFieldEqRespond) {
    // test for sync rpc_mock
    m_response.set_count(2);
    EXPECT_RPC("poppy.examples.mock_rpc.WordCountService.Count",
               poppy::SingularFieldEq("text", std::string("find jobs")))
            .WillOnce(poppy::Respond(m_response));
    SyncCallCheck("find jobs", 2);

    // test for async rpc_mock
    EXPECT_RPC("poppy.examples.mock_rpc.WordCountService.Count",
               poppy::SingularFieldEq("text", std::string("find jobs")))
            .WillOnce(poppy::Respond(m_response));
    AsyncCallCheck("find jobs", 2);
}

TEST_F(MockRPCTest, SingularFieldEqFailReason) {
    // test for sync rpc_mock
    m_response.set_count(-1);
    EXPECT_RPC("poppy.examples.mock_rpc.WordCountService.Count",
               poppy::SingularFieldEq("text", std::string("~")))
            .WillOnce(poppy::FailReason("unknown words"));
    SyncCallCheck("~", -1);

    // test for async rpc_mock
    EXPECT_RPC("poppy.examples.mock_rpc.WordCountService.Count",
               poppy::SingularFieldEq("text", std::string("~")))
            .WillOnce(poppy::FailReason("unknown words"));
    AsyncCallCheck("~", -1);
}

TEST_F(MockRPCTest, SingularFieldEqFailCode) {
    m_response.set_count(-1);

    // test for sync rpc_mock
    EXPECT_RPC("poppy.examples.mock_rpc.WordCountService.Count",
               poppy::SingularFieldEq("text", std::string("~")))
            .WillOnce(poppy::FailCode(RPC_ERROR_CONNECTION_CLOSED));
    SyncCallCheck("~", -1);

    // test for async rpc_mock
    EXPECT_RPC("poppy.examples.mock_rpc.WordCountService.Count",
               poppy::SingularFieldEq("text", std::string("~")))
            .WillOnce(poppy::FailCode(RPC_ERROR_CONNECTION_CLOSED));
    AsyncCallCheck("~", -1);
}

TEST_F(MockRPCTest, SingularFieldEqFailCodeRequestTimeout) {
    m_response.set_count(-1);

    // test for sync rpc_mock
    EXPECT_RPC("poppy.examples.mock_rpc.WordCountService.Count",
               poppy::SingularFieldEq("text", std::string("~")))
            .WillOnce(poppy::FailCode(RPC_ERROR_REQUEST_TIMEOUT));
    SyncCallCheck("~", -1);

    // test for async rpc_mock
    EXPECT_RPC("poppy.examples.mock_rpc.WordCountService.Count",
               poppy::SingularFieldEq("text", std::string("~")))
            .WillOnce(poppy::FailCode(RPC_ERROR_REQUEST_TIMEOUT));
    AsyncCallCheck("~", -1);
}
}  // namespace mock_rpc
}  // namespace examples
}  // namespace poppy

int main(int argc, char* argv[]) {
    ::testing::InitGoogleMock(&argc, argv);
    ::InitAllModulesAndTest(&argc, &argv, true);
    return RUN_ALL_TESTS();
}

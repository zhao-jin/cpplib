// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: Simon Wang <simonwang@tencent.com>
// Created: 12/19/11
// Description: Client GTest's Wrapper

#include <sstream>
#include <string>

#include "common/base/singleton.h"
#include "common/system/concurrency/this_thread.h"

#include "poppy/test/echo_test.h"
#include "poppy/test/server_controller.h"

#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/gtest/gtest.h"

DEFINE_bool(use_agent, false, "whether or not use agent");
DEFINE_string(agent_listen_ip, "127.0.0.1", "agent listen address");
DEFINE_int32(agent_listen_port, 50004, "agent listen port");

namespace poppy {
namespace test {

const int kConstructRoundCount = 10;
const int kTestServerNum = 2;

class PoppyConnectionTest : public testing::Test
{
protected:
    virtual void SetUp() {
        rpc_examples::EchoClient::s_crash_on_timeout = false;
        LaunchOptions options;
        options.use_agent = FLAGS_use_agent;
        options.agent_listen_ip = FLAGS_agent_listen_ip;
        options.agent_listen_port = FLAGS_agent_listen_port;
        options.server_listen_port[0] = 0;
        options.server_listen_port[1] = 0;

        m_server_controller = new ServerController(options);
        for (int i = 0; i < kTestServerNum; ++i) {
            StartServer(i);
        }
    }

    virtual void TearDown() {
        delete m_server_controller;
    }

    std::string GetServerAddress(int index) {
        return m_server_controller->GetServerAddress(index);
    }

    bool StartServer(int index, bool use_credential = false,
                     common::CredentialVerifier* credential_verifier = NULL) {
        return m_server_controller->StartServer(index, use_credential, credential_verifier);
    }

    bool StopServer(int index) {
        return m_server_controller->StopServer(index);
    }

    bool EnableServer(int index) {
        return m_server_controller->EnableServer(index);
    }

    bool DisableServer(int index) {
        return m_server_controller->DisableServer(index);
    }

protected:
    ServerController* m_server_controller;
};

TEST_F(PoppyConnectionTest, Basic)
{
    rpc_examples::EchoClient client(GetServerAddress(0));
    int throughput = 0;

    throughput = client.RunLatencyTest(1);
    VLOG(3) << "Latency test: " << throughput << "/s\n";

    throughput = client.RunReadTest(1);
    VLOG(3) << "Read throughput: " << throughput << "M/s\n";
    EXPECT_GT(throughput, 0) << "Error: Package lost.";

    throughput = client.RunWriteTest(1);
    VLOG(3) << "Write throughput: " << throughput << "M/s\n";
    EXPECT_GT(throughput, 0) << "Error: Package lost.";

    throughput = client.RunReadWriteTest(1);
    VLOG(3) << "Read/Write throughput: " << throughput << "M/s\n";
    EXPECT_GT(throughput, 0) << "Error: Package lost.";

    client.RunAsyncTest(100);
}

TEST_F(PoppyConnectionTest, WrongAddress)
{
    std::string invalid_address = m_server_controller->GetInvalidAddress();
    rpc_examples::EchoClient client(invalid_address);
    int throughput = 0;

    throughput = client.RunLatencyTest(1);
    VLOG(3) << "Latency test: " << throughput << "/s\n";

    throughput = client.RunReadTest(1);
    VLOG(3) << "Read throughput: " << throughput << "M/s\n";
    EXPECT_EQ(throughput, -1) << "Error: Package lost.";

    throughput = client.RunWriteTest(1);
    VLOG(3) << "Write throughput: " << throughput << "M/s\n";
    EXPECT_EQ(throughput, -1) << "Error: Package lost.";

    throughput = client.RunReadWriteTest(1);
    VLOG(3) << "Read/Write throughput: " << throughput << "M/s\n";
    EXPECT_EQ(throughput, -1) << "Error: Package lost.";

    client.RunAsyncTest(1);
}

TEST_F(PoppyConnectionTest, ConstructDelete)
{
    // just construct and destruct
    for (int i = 0; i < kConstructRoundCount; ++i) {
        rpc_examples::EchoClient client(GetServerAddress(0));
        poppy::RpcClient rpc_client;
        poppy::RpcChannel channel(&rpc_client, GetServerAddress(0));
        rpc_examples::TestServer server;
    }

    // test whether client can exit normally or not during operation
    for (int i = 0; i < kConstructRoundCount; ++i) {
        rpc_examples::EchoClient client(GetServerAddress(0));
        client.RunLatencyTest(1);
    }

    for (int i = 0; i < kConstructRoundCount; ++i) {
        rpc_examples::EchoClient client(GetServerAddress(0));
        client.RunReadTest(1);
    }

    for (int i = 0; i < kConstructRoundCount; ++i) {
        rpc_examples::EchoClient client(GetServerAddress(0));
        client.RunReadTest(1);
    }

    for (int i = 0; i < kConstructRoundCount; ++i) {
        rpc_examples::EchoClient client(GetServerAddress(0));
        client.RunWriteTest(1);
    }

    for (int i = 0; i < kConstructRoundCount; ++i) {
        rpc_examples::EchoClient client(GetServerAddress(0));
        client.RunReadWriteTest(1);
    }
}

TEST_F(PoppyConnectionTest, DynamicAddress)
{
    rpc_examples::EchoClient client(GetServerAddress(0));
    int throughput = 0;

    throughput = client.RunLatencyTest(1);
    VLOG(3) << "Latency test: " << throughput << "/s\n";

    throughput = client.RunReadTest(5);
    VLOG(3) << "Read throughput: " << throughput << "M/s\n";
    EXPECT_GT(throughput, 0) << "Error: Package lost.";

    client.ChangeAddresses(GetServerAddress(1));

    throughput = client.RunWriteTest(5);
    VLOG(3) << "Write throughput: " << throughput << "M/s\n";
    EXPECT_GT(throughput, 0) << "Error: Package lost.";

    throughput = client.RunReadWriteTest(5);
    VLOG(3) << "Read/Write throughput: " << throughput << "M/s\n";
    EXPECT_GT(throughput, 0) << "Error: Package lost.";

    client.RunAsyncTest(1);
}

TEST_F(PoppyConnectionTest, TimeoutAll)
{
    rpc_examples::EchoClient client(GetServerAddress(0));
    ASSERT_TRUE(StopServer(0));
    client.RunAsyncTest(10);
}

TEST_F(PoppyConnectionTest, TimeoutFirstHalf)
{
    rpc_examples::EchoClient client(GetServerAddress(0));
    ASSERT_TRUE(StopServer(0));
    client.SendAsyncTestRequest(5);
    ASSERT_TRUE(StartServer(0));
    client.WaitForAsyncTestFinish();
    VLOG(3) << client.Dump();
}

TEST_F(PoppyConnectionTest, AsyncWithFailureServer)
{
    std::string server_address = GetServerAddress(0)
                                 + "," + GetServerAddress(1);
    rpc_examples::EchoClient client(server_address);

    client.SendAsyncTestRequest(10);
    ASSERT_TRUE(StopServer(0));
    client.WaitForAsyncTestFinish();
    client.SendAsyncTestRequest(10);
    ASSERT_TRUE(StopServer(1));
    ASSERT_TRUE(StartServer(0));
    client.WaitForAsyncTestFinish();
}

TEST_F(PoppyConnectionTest, DeleteChannel)
{
    std::string server_address = GetServerAddress(0);
    rpc_examples::EchoClient* client =
        new rpc_examples::EchoClient(server_address);

    client->SendAsyncTestRequest(50);
    client->Shutdown(true);
    delete client;
}

TEST_F(PoppyConnectionTest, Healthy)
{
    rpc_examples::EchoClient client(GetServerAddress(0));
    rpc_examples::RpcChannel* channel = client.GetChannel();

    while (channel->GetChannelStatus() != poppy::ChannelStatus_Healthy) {
        VLOG(3) << "Wait channel status to be Healthy\n";
        ThisThread::Sleep(10);
    }

    LOG(INFO) << "channel status is: " << static_cast<int>(channel->GetChannelStatus());

    // normal case.
    client.SendAsyncTestRequest(1);
    client.WaitForAsyncTestFinish();

    // set server to not healthy, following 5 packages should fail.
    ASSERT_TRUE(DisableServer(0));
    ThisThread::Sleep(100);

    LOG(INFO) << "channel status is: " << static_cast<int>(channel->GetChannelStatus());

    client.SendAsyncTestRequest(2);
    client.WaitForAsyncTestFinish();

    // send the request, and enable the server later, these 5 request should be OK.
    client.SendAsyncTestRequest(2);
    ASSERT_TRUE(EnableServer(0));
    client.WaitForAsyncTestFinish();

    ThisThread::Sleep(100);
    LOG(INFO) << "channel status is: " << static_cast<int>(channel->GetChannelStatus());

    client.SendAsyncTestRequest(1);
    client.WaitForAsyncTestFinish();
}

TEST_F(PoppyConnectionTest, RejectVerify)
{
    rpc_examples::EchoClient client(GetServerAddress(0));
    ASSERT_TRUE(StopServer(0));
    ASSERT_TRUE(StartServer(0, true,
                            &Singleton<rpc_examples::RejectCredentialVerifier>::Instance()));
    client.RunAsyncTest(5);
}

} // namespace test
} // namespace poppy

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    FLAGS_log_dir = ".";
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

    return RUN_ALL_TESTS();
}

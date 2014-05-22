// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: Dongping Huang <dphuang@tencent.com>
// Created: 06/13/11
// Description:

#ifndef POPPY_TEST_ECHO_TEST_H
#define POPPY_TEST_ECHO_TEST_H
#pragma once

#include <string>
#include <vector>
#include "common/base/string/algorithm.h"
#include "common/base/string/string_number.h"
#include "common/system/concurrency/atomic/atomic.h"
#include "common/system/concurrency/thread.h"
#include "poppy/rpc_channel.h"
#include "poppy/rpc_client.h"
#include "poppy/rpc_handler.h"
#include "poppy/rpc_http_channel.h"
#include "poppy/test/echo_service.pb.h"
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/gtest/gtest.h"

namespace rpc_examples
{
// using namespace common;

class RpcChannel : public poppy::RpcHttpChannel
{
public:
    RpcChannel(poppy::RpcClient* rpc_client,
               const std::string& name,
               const poppy::RpcChannelOptions options)
        : poppy::RpcHttpChannel(rpc_client->impl(), name, NULL, options)
    {
    }

    void OnAddressesChanged(const std::string& address)
    {
        std::vector<std::string> servers;
        SplitString(address, ";", &servers);
        m_channel_impl->OnAddressesChanged(servers);
    }
    std::string Dump() const
    {
        return m_channel_impl->Dump();
    }
};

class EchoClient
{
public:
    explicit EchoClient(
        const std::string& server_name,
        const poppy::RpcChannelOptions& options = poppy::RpcChannelOptions(),
        bool share_rpc_client = false):
        m_server_name(server_name)
    {
        if (share_rpc_client) {
            m_rpc_client = new poppy::RpcClient();
        } else {
            m_rpc_client = new poppy::RpcClient(poppy::RpcClientOptions());
        }
        m_rpc_channel = new RpcChannel(m_rpc_client, m_server_name, options);
    }

    ~EchoClient()
    {
        delete m_rpc_channel;
        delete m_rpc_client;
    }

    void Shutdown(bool wait_all_pending_request_finish = false)
    {
        m_rpc_client->Shutdown(wait_all_pending_request_finish);
    }

    RpcChannel* GetChannel() {
        return m_rpc_channel;
    }

    void ChangeAddresses(const std::string& address)
    {
        m_rpc_channel->OnAddressesChanged(address);
    }

    int RunLatencyTest(uint32_t round, int32_t timeout = 0)
    {
        rpc_examples::EchoServer::Stub echo_client(m_rpc_channel);
        int64_t t;
        t = GetTimeStampInUs();

        for (uint32_t i = 0; i < round; ++i) {
            poppy::RpcController rpc_controller;
            rpc_examples::EchoRequest request;
            rpc_examples::EchoResponse response;
            request.set_user(IntegerToString(i));
            request.set_message("t");
            request.set_timeout(timeout);
            echo_client.Echo(&rpc_controller, &request, &response, NULL);

            if (rpc_controller.Failed()) {
                LogOnRpcFailed(rpc_controller);
            }
        }

        t = GetTimeStampInUs() - t;
        int64_t throughput = round / (t / 1000000.0);
        return throughput;
    }

    int RunReadTest(uint32_t round, int32_t timeout = 0)
    {
        rpc_examples::EchoServer::Stub echo_client(m_rpc_channel);
        int64_t t;
        t = GetTimeStampInUs();

        for (uint32_t i = 0; i < round; ++i) {
            poppy::RpcController rpc_controller;
            rpc_examples::EchoRequest request;
            rpc_examples::EchoResponse response;
            request.set_user("t");
            request.set_message("t");
            request.set_timeout(timeout);

            echo_client.Get(&rpc_controller, &request, &response, NULL);

            if (rpc_controller.Failed()) {
                LogOnRpcFailed(rpc_controller);
                return -1;
            }
        }

        t = GetTimeStampInUs() - t;
        int64_t throughput = round / (t / 1000000.0);
        return throughput;
    }

    int RunWriteTest(uint32_t round, int32_t timeout = 0)
    {
        rpc_examples::EchoServer::Stub echo_client(m_rpc_channel);
        int64_t t;
        t = GetTimeStampInUs();

        for (uint32_t i = 0; i < round; ++i) {
            poppy::RpcController rpc_controller;
            rpc_examples::EchoRequest request;
            rpc_examples::EchoResponse response;
            request.set_user("t");
            request.set_message(std::string(1000 * 1024, 'A'));
            request.set_timeout(timeout);

            echo_client.Set(&rpc_controller, &request, &response, NULL);

            if (rpc_controller.Failed()) {
                LogOnRpcFailed(rpc_controller);
                return -1;
            }
        }

        t = GetTimeStampInUs() - t;
        int64_t throughput = round / (t / 1000000.0);
        return throughput;
    }

    int RunReadWriteTest(uint32_t round, int32_t timeout = 0)
    {
        rpc_examples::EchoServer::Stub echo_client(m_rpc_channel);
        int64_t t;
        t = GetTimeStampInUs();

        for (uint32_t i = 0; i < round; ++i) {
            poppy::RpcController rpc_controller;
            rpc_examples::EchoRequest request;
            rpc_examples::EchoResponse response;
            request.set_user("t");
            request.set_message(std::string(1000 * 1024, 'A'));
            request.set_timeout(timeout);

            echo_client.Echo(&rpc_controller, &request, &response, NULL);

            if (rpc_controller.Failed()) {
                LogOnRpcFailed(rpc_controller);
                return -1;
            }
        }

        int64_t t2 = GetTimeStampInUs();
        t = t2 - t;
        int64_t throughput = 2 * round / (t / 1000000.0);
        return throughput;
    }

    static bool s_crash_on_timeout;

    static void EchoCallback(poppy::RpcController* controller,
                             rpc_examples::EchoRequest* request,
                             rpc_examples::EchoResponse* response)
    {
        if (controller->Failed()) {
            LogOnRpcFailed(*controller);
            static int count = 0;
            ++count;
        }

        delete controller;
        delete request;
        delete response;
        --s_request_count;
    }

    int RunAsyncTest(uint32_t round, int timeout = 0, bool wait_finished = true)
    {
        SendAsyncTestRequest(round, timeout);
        if (wait_finished)
            WaitForAsyncTestFinish();
        return 0;
    }

    int SendAsyncTestRequest(uint32_t round, int timeout = 0)
    {
        rpc_examples::EchoServer::Stub echo_client(m_rpc_channel);
        s_request_count = 0;

        for (uint32_t i = 0; i < round; ++i) {
            poppy::RpcController* rpc_controller =
                new poppy::RpcController();
            rpc_examples::EchoRequest* request = new rpc_examples::EchoRequest();
            rpc_examples::EchoResponse* response = new rpc_examples::EchoResponse();
            google::protobuf::Closure* done =
                NewClosure(&EchoClient::EchoCallback,
                           rpc_controller,
                           request,
                           response);
            request->set_user("t");
            request->set_timeout(timeout);
            ++s_request_count;

            switch (i % 4) {
            case 0:
                rpc_controller->SetRequestCompressType(poppy::CompressTypeSnappy);
                rpc_controller->SetResponseCompressType(poppy::CompressTypeSnappy);
                request->set_message("t");
                echo_client.Echo(rpc_controller, request, response, done);
                break;
            case 1:
                request->set_message("t");
                echo_client.Get(rpc_controller, request, response, done);
                break;
            case 2:
                request->set_message(std::string(1000 * 1024, 'a'));
                echo_client.Set(rpc_controller, request, response, done);
                break;
            case 3:
                request->set_message(std::string(1000 * 1024, 'a'));
                echo_client.Echo(rpc_controller, request, response, done);
                break;
            }

            if (i % 100 == 99) {
                ThisThread::Sleep(1000);
            }
        }

        return 0;
    }

    int WaitForAsyncTestFinish()
    {
        for (;;) {
            ThisThread::Sleep(100);

            if (s_request_count == 0) {
                break;
            }
        }

        return 0;
    }

    std::string Dump() const
    {
        return m_rpc_channel->Dump();
    }

private:
    static void LogOnRpcFailed(const poppy::RpcController& controller)
    {
        if (s_crash_on_timeout) {
            LOG(FATAL) << "failed: " << controller.ErrorText();
        } else {
            VLOG(1) << "failed: " << controller.ErrorText();
        }
    }

private:
    std::string m_server_name;
    poppy::RpcClient* m_rpc_client;
    RpcChannel* m_rpc_channel;
    static Atomic<int> s_request_count;
};

Atomic<int> EchoClient::s_request_count;
bool EchoClient::s_crash_on_timeout = true;

} // namespace rpc_examples


#endif // POPPY_TEST_ECHO_TEST_H

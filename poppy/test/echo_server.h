// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: Dongping Huang <dphuang@tencent.com>
// Created: 06/10/11
// Description:

#ifndef POPPY_TEST_ECHO_SERVER_H
#define POPPY_TEST_ECHO_SERVER_H
#pragma once

// An example echo server.

#include <signal.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include "common/base/export_variable.h"
#include "common/base/scoped_ptr.h"
#include "common/crypto/credential/verifier.h"
#include "common/system/concurrency/atomic/atomic.h"
#include "common/system/concurrency/thread.h"
#include "common/system/concurrency/thread_pool.h"
#include "poppy/rpc_controller.h"
#include "poppy/rpc_handler.h"
#include "poppy/rpc_server.h"
#include "poppy/test/echo_service.pb.h"

namespace rpc_examples {

// using namespace common;

class RejectCredentialVerifier : public common::CredentialVerifier {
public:
    void Verify(const std::string& cred_str, const std::string& client_address,
                VerifyFinishedCallback* done)
    {
        const std::string user_id = "";
        const std::string user_role = "";
        done->Run(&user_id, &user_role, 1);
    }

    bool IsRejected(int error_code)
    {
        return true;
    }
};

class EchoServerImpl : public EchoServer
{
public:
    EchoServerImpl() : m_simple_request_count(0) {}
    virtual ~EchoServerImpl() {}

    int GetAndClearRequestCount()
    {
        return m_simple_request_count.Exchange(0);
    }

private:
    virtual void Echo(google::protobuf::RpcController* controller,
                      const EchoRequest* request,
                      EchoResponse* response,
                      google::protobuf::Closure* done)
    {
        ++m_simple_request_count;
        if (request->timeout() != 0) {
            ThisThread::Sleep(request->timeout());
        }
        response->set_user(request->user());
        response->set_message(request->message());
        // VLOG(3) << "receive Echo rpc call";
        done->Run();
    }

    virtual void Get(google::protobuf::RpcController* controller,
                     const EchoRequest* request,
                     EchoResponse* response,
                     google::protobuf::Closure* done)
    {
        ++m_simple_request_count;
        if (request->timeout() != 0) {
            ThisThread::Sleep(request->timeout());
        }
        std::string s(1000 * 1024, 'A');
        response->set_message(s);
        response->set_user(request->user());
        done->Run();
    }

    virtual void Set(google::protobuf::RpcController* controller,
                     const EchoRequest* request,
                     EchoResponse* response,
                     google::protobuf::Closure* done)
    {
        ++m_simple_request_count;
        if (request->timeout() != 0) {
            ThisThread::Sleep(request->timeout());
        }
        std::string s(1, 'A');
        response->set_message(s);
        response->set_user(request->user());
        done->Run();
    }

    Atomic<int> m_simple_request_count;
};

class TestServer : public poppy::RpcServer
{
public:
    explicit TestServer(
        int thread_number = 0,
        common::CredentialVerifier* credential_verifier = NULL) :
        poppy::RpcServer(thread_number, credential_verifier),
        m_server_impl(new EchoServerImpl())
    {
        this->RegisterService(m_server_impl);
        m_healthy = true;
    }
    explicit TestServer(
        netframe::NetFrame *netframe,
        common::CredentialVerifier* credential_verifier = NULL) :
        poppy::RpcServer(netframe, true, credential_verifier),
        m_server_impl(new EchoServerImpl())
    {
        this->RegisterService(m_server_impl);
        m_healthy = true;
    }
    ~TestServer() {}
    int GetAndClearRequestCount()
    {
        return m_server_impl->GetAndClearRequestCount();
    }
    virtual bool IsHealthy()
    {
        return m_healthy;
    }
    void SetHealthy(bool healthy)
    {
        m_healthy = healthy;
    }

private:
    EchoServerImpl* m_server_impl;
    bool m_healthy;
};

} // namespace rpc_examples

#endif // POPPY_TEST_ECHO_SERVER_H

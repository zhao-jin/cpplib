// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: Dongping HUANG <dphuang@tencent.com>
// Created: 06/23/11
// Description:

#include <signal.h>
#include "common/crypto/random/pseudo_random.h"
#include "common/system/concurrency/atomic/atomic.h"
#include "common/system/concurrency/this_thread.h"
#include "poppy/rpc_channel.h"
#include "poppy/rpc_client.h"
#include "poppy/test/echo_service.pb.h"
// includes from thirdparty
#include "gflags/gflags.h"
#include "glog/logging.h"

// using namespace common;

DEFINE_string(server_address, "127.0.0.1:50000", "server address");
DEFINE_int32(request_number, 300, "request number");
DEFINE_string(request_type, "random", "request type");
DEFINE_bool(crash_on_timeout, false, "crash on timeout");

Atomic<int> g_request_count = 0;
Atomic<int> g_total_request_count = 0;
PseudoRandom g_rand_gen = PseudoRandom(0);

void SendRequest(rpc_examples::EchoServer::Stub* echo_client,
                 int type,
                 poppy::RpcController *rpc_controller);

bool g_quit = false;
int request_type = -1;

void SignalIntHandler(int sig)
{
    g_quit = true;
}

namespace rpc_examples {

class ConnectionManager
{
public:
    static ConnectionManager* instance() {
        return &s_instance;
    }
    void SetServerAddress(const std::string& servers) {
        m_server_addresses = servers;
    }
    poppy::RpcChannel* GetChannel() {
        if (!m_rpc_channel) {
            m_rpc_channel =
                new poppy::RpcChannel(&m_rpc_client, m_server_addresses);
        }

        return m_rpc_channel;
    }
    void GetMemoryUsage(poppy::MemoryUsage* memory_usage) const {
        return m_rpc_client.GetMemoryUsage(memory_usage);
    }
private:
    ConnectionManager() : m_rpc_channel(0) {}
    ~ConnectionManager() {
        delete m_rpc_channel;
    }

private:
    static ConnectionManager s_instance;
    std::string m_server_addresses;
    poppy::RpcClient m_rpc_client;
    poppy::RpcChannel* m_rpc_channel;
};

} // namespace rpc_examples

rpc_examples::ConnectionManager rpc_examples::ConnectionManager::s_instance;

void EchoCallback(rpc_examples::EchoServer::Stub* client,
                  poppy::RpcController* controller,
                  rpc_examples::EchoRequest* request,
                  rpc_examples::EchoResponse* response)
{
    if (controller->Failed()) {
        if (FLAGS_crash_on_timeout) {
            LOG(FATAL) << "failed: " << controller->ErrorText();
        } else {
            VLOG(1) << "failed: " << controller->ErrorText();
        }
    }

    delete request;
    delete response;
    --g_request_count;

    if (!g_quit) {
        if (request_type >= 0) {
            SendRequest(client, request_type, controller);
        } else {
            int type = g_rand_gen.NextUInt32() % 4;
            SendRequest(client, type, controller);
        }
    } else {
        delete controller;
    }
}

void SendRequest(rpc_examples::EchoServer::Stub* echo_client,
                 int type,
                 poppy::RpcController *rpc_controller)
{
    rpc_examples::EchoRequest* request = new rpc_examples::EchoRequest();
    rpc_examples::EchoResponse* response = new rpc_examples::EchoResponse();
    google::protobuf::Closure* done =
        NewClosure(&EchoCallback,
                   echo_client,
                   rpc_controller,
                   request,
                   response);
    request->set_user("t");
    ++g_request_count;
    ++g_total_request_count;

    if (g_total_request_count % 1000 == 0) {
        poppy::MemoryUsage memory;
        rpc_examples::ConnectionManager::instance()->GetMemoryUsage(&memory);
        VLOG(3) << "queued size: " << memory.queued_size
            << ", send_buffer size: " << memory.send_buffer_size
            << ", receive_buffer size: " << memory.receive_buffer_size;
    }

    switch (type) {
    case 0:  // latency test
        request->set_message("t");
        echo_client->Echo(rpc_controller, request, response, done);
        break;
    case 1:  // read test
        request->set_message("t");
        echo_client->Get(rpc_controller, request, response, done);
        break;
    case 2:  // write test
        request->set_message(std::string(1000 * 1024, 'a'));
        echo_client->Set(rpc_controller, request, response, done);
        break;
    case 3:  // read write test
        request->set_message(std::string(1000 * 1024, 'a'));
        echo_client->Echo(rpc_controller, request, response, done);
        break;
    }
}

int main(int argc, char** argv)
{
    google::InitGoogleLogging(argv[0]);

    FLAGS_v = 3;
    FLAGS_log_dir = ".";
    google::ParseCommandLineFlags(&argc, &argv, true);

    signal(SIGINT, SignalIntHandler);
    signal(SIGTERM, SignalIntHandler);

    rpc_examples::ConnectionManager::instance()->SetServerAddress(
            FLAGS_server_address);
    rpc_examples::EchoServer::Stub
    echo_client(rpc_examples::ConnectionManager::instance()->GetChannel());

    if (FLAGS_request_type == "latency") {
        request_type = 0;
    } else if (FLAGS_request_type == "read") {
        request_type = 1;
    } else if (FLAGS_request_type == "write") {
        request_type = 2;
    } else if (FLAGS_request_type == "read_write") {
        request_type = 3;
    }

    for (int32_t i = 0; i < FLAGS_request_number; ++i) {
        if (request_type >= 0) { // send a specified type of request
            poppy::RpcController* rpc_controller = new poppy::RpcController();
            SendRequest(&echo_client, request_type, rpc_controller);
        } else { // send a random type of request
            poppy::RpcController* rpc_controller = new poppy::RpcController();
            SendRequest(&echo_client, i % 4, rpc_controller);
        }
    }

    for (;;) {
        ThisThread::Sleep(100);

        if (g_quit && g_request_count == 0) {
            break;
        }
    }

    LOG(INFO) << "All response is back, now quit...";

    return EXIT_SUCCESS;
}


// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>
// Eric Liu <ericliu@tencent.com>
//
// An example echo sync client.

#include <iostream>
#include "common/base/string/algorithm.h"
#include "common/system/time/stopwatch.h"

// rpc client related headers.
#include "poppy/examples/echo_sync/echo_service.pb.h"
#include "poppy/rpc_channel.h"
#include "poppy/rpc_client.h"

// includes from thirdparty
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"

// using namespace common;

DEFINE_string(server_address, "127.0.0.1:10000", "the address server listen on.");

void SyncSingleCall(rpc_examples::EchoServer::Stub* echo_stub)
{
    poppy::RpcController rpc_controller;

    rpc_examples::EchoRequest request;
    request.set_user("echo_test_user");
    request.set_message("hello");

    rpc_examples::EchoResponse response;

    echo_stub->EchoMessage(&rpc_controller, &request, &response, NULL);
    if (rpc_controller.Failed()) {
        LOG(INFO) << "failed: " << rpc_controller.ErrorText();
    } else {
        std::cout << "response: " << response.DebugString();
    }
}

void SyncRepeatCall(rpc_examples::EchoServer::Stub* echo_stub)
{
    poppy::RpcController rpc_controller;

    rpc_examples::EchoRequest request;
    request.set_user("echo_test_user");
    request.set_message("hello");

    rpc_examples::EchoResponse response;

    const int kLoopCount = 1000;
    Stopwatch stopwatch;
    for (int i = 0; i < kLoopCount; ++i) {
        echo_stub->EchoMessage(&rpc_controller, &request, &response, NULL);
        if (rpc_controller.Failed()) {
            LOG(INFO) << "failed: " << rpc_controller.ErrorText();
        }
    }
    double latency = static_cast<double>(stopwatch.ElapsedMilliSeconds()) / kLoopCount;
    std::cout << "SyncRepeatCall complete\n"
              << "latency: " << latency << "us\n";
}

int main(int argc, char** argv)
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

    // Define an rpc client. It's shared by all rpc channels.
    poppy::RpcClient rpc_client;

    // Define an rpc channel. It connects to a specified group of servers.
    poppy::RpcChannel rpc_channel(&rpc_client, FLAGS_server_address);

    // Define an rpc stub. It use a specifed channel to send requests.
    rpc_examples::EchoServer::Stub echo_stub(&rpc_channel);

    SyncSingleCall(&echo_stub);
    SyncRepeatCall(&echo_stub);

    return EXIT_SUCCESS;
}

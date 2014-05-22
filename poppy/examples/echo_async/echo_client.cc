// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>
// Eric Liu <ericliu@tencent.com>
//
// An example echo sync client.

#include "common/base/string/algorithm.h"
#include "common/system/concurrency/this_thread.h"

// rpc client related headers.
#include "poppy/examples/echo_async/echo_service.pb.h"
#include "poppy/rpc_channel.h"
#include "poppy/rpc_client.h"

// include concurrency related headers.
#include "common/system/concurrency/atomic/atomic.h"

// includes from thirdparty
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"

// using namespace common;

DEFINE_string(server_address, "127.0.0.1:10000", "the address server listen on.");

Atomic<int> g_request_count;

void EchoCallback(poppy::RpcController* controller,
                  rpc_examples::EchoRequest* request,
                  rpc_examples::EchoResponse* response)
{
    if (controller->Failed()) {
        LOG(INFO) << "failed: " << controller->ErrorText();
    } else {
        LOG(INFO) << "response: " << response->message();
    }

    delete controller;
    delete request;
    delete response;
    --g_request_count;
}

int main(int argc, char** argv)
{
    google::ParseCommandLineFlags(&argc, &argv, true);

    // Define an rpc client. It's shared by all rpc channels.
    poppy::RpcClient rpc_client;

    // Define an rpc channel. It connects to a specified group of servers.
    poppy::RpcChannelOptions options;
    options.set_tos(148);
    poppy::RpcChannel rpc_channel(&rpc_client, FLAGS_server_address, NULL, options);

    // Define an rpc stub. It use a specifed channel to send requests.
    rpc_examples::EchoServer::Stub echo_client(&rpc_channel);

    poppy::RpcController* rpc_controller = new poppy::RpcController();
    rpc_examples::EchoRequest* request = new rpc_examples::EchoRequest();
    rpc_examples::EchoResponse* response = new rpc_examples::EchoResponse();
    google::protobuf::Closure* done =
        NewClosure(&EchoCallback, rpc_controller, request, response);

    request->set_user("echo_test_user");
    request->set_message("hello");

    LOG(INFO) << "sending request: " << request->message();

    ++g_request_count;
    echo_client.Echo(rpc_controller, request, response, done);

    for (;;) {
        ThisThread::Sleep(100);

        if (g_request_count == 0) {
            break;
        }
    }

    return EXIT_SUCCESS;
}

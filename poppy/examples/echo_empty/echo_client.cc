// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>
// Eric Liu <ericliu@tencent.com>
//
// An example echo sync client.

#include "common/base/string/algorithm.h"

// rpc client related headers.
#include "poppy/examples/echo_empty/echo_service.pb.h"
#include "poppy/rpc_channel.h"
#include "poppy/rpc_client.h"

// includes from thirdparty
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"

// using namespace common;

DEFINE_string(server_address, "127.0.0.1:10000", "the address server listen on.");

int main(int argc, char** argv)
{
    google::ParseCommandLineFlags(&argc, &argv, true);

    // Define an rpc client. It's shared by all rpc channels.
    poppy::RpcClient rpc_client;

    // Define an rpc channel. It connects to a specified group of servers.
    poppy::RpcChannel rpc_channel(&rpc_client, FLAGS_server_address);

    // Define an rpc stub. It use a specifed channel to send requests.
    rpc_examples::EchoServer::Stub echo_client(&rpc_channel);

    // Send an rpc request with empty message.
    {
        poppy::RpcController rpc_controller;
        poppy::EmptyRequest request;
        rpc_examples::EchoResponse response;
        LOG(INFO) << "sending empty request";
        echo_client.EmptyParamEcho(&rpc_controller, &request, &response, NULL);

        if (rpc_controller.Failed()) {
            LOG(INFO) << "failed: " << rpc_controller.ErrorText();
        } else {
            LOG(INFO) << "response: " << response.message();
        }
    }

    // Send an rpc request and it returns an empty message.
    {
        poppy::RpcController rpc_controller;
        rpc_examples::EchoRequest request;
        poppy::EmptyResponse response;
        request.set_user("echo_test_user");
        request.set_message("hello");
        LOG(INFO) << "sending request: " << request.message();
        echo_client.EmptyReturnEcho(&rpc_controller, &request, &response, NULL);

        if (rpc_controller.Failed()) {
            LOG(INFO) << "failed: " << rpc_controller.ErrorText();
        } else {
            LOG(INFO) << "received an empty response.";
        }
    }

    // Send an rpc request with empty message and it returns an empty message.
    {
        poppy::RpcController rpc_controller;
        poppy::EmptyRequest request;
        poppy::EmptyResponse response;
        LOG(INFO) << "sending empty request";
        echo_client.EmptyEcho(&rpc_controller, &request, &response, NULL);

        if (rpc_controller.Failed()) {
            LOG(INFO) << "failed: " << rpc_controller.ErrorText();
        } else {
            LOG(INFO) << "received an empty response.";
        }
    }

    return EXIT_SUCCESS;
}
